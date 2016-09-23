#include "hotkey.h"
#include "locale.h"
#include "parse.h"
#include <string.h>
#include <mach/mach_time.h>

#define internal static
#define local_persist static
#define CLOCK_PRECISION 1E-9

extern mode DefaultBindingMode;
extern mode *ActiveBindingMode;
extern uint32_t Compatibility;

internal inline void
ClockGetTime(long long *Time)
{
    local_persist mach_timebase_info_data_t Timebase;
    if(Timebase.denom == 0)
    {
        mach_timebase_info(&Timebase);
    }

    uint64_t Temp = mach_absolute_time();
    *Time = (Temp * Timebase.numer) / Timebase.denom;
}

internal inline double
GetTimeDiff(long long Time)
{
    return (Time - ActiveBindingMode->Time) * CLOCK_PRECISION;
}

internal inline void
UpdatePrefixTimer()
{
    ClockGetTime(&ActiveBindingMode->Time);
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, ActiveBindingMode->Timeout * NSEC_PER_SEC),
                   dispatch_get_main_queue(),
    ^{
        if(ActiveBindingMode->Prefix)
        {
            long long Time;
            ClockGetTime(&Time);

            if(GetTimeDiff(Time) >= ActiveBindingMode->Timeout)
            {
                if(ActiveBindingMode->Restore)
                    ActivateMode(ActiveBindingMode->Restore);
                else
                    ActivateMode("default");
            }
        }
    });
}

/* TODO(koekeishiya): We probably want the user to be able to specify
 * which shell they want to use. */
internal const char *Shell = "/bin/bash";
internal const char *ShellArgs = "-c";

void Execute(char *Command)
{
    int ChildPID = fork();
    if(ChildPID == 0)
    {
        char *Exec[] = { (char *) Shell, (char *) ShellArgs, Command, NULL};
        int StatusCode = execvp(Exec[0], Exec);
        exit(StatusCode);
    }

    if(ActiveBindingMode->Prefix)
        UpdatePrefixTimer();
}

internal inline void
ExecuteKwmBorderCommand()
{
    char KwmCommand[64] = "kwmc config border focused color ";
    strcat(KwmCommand, ActiveBindingMode->Color);

    int ChildPID = fork();
    if(ChildPID == 0)
    {
        char *Exec[] = { (char *) Shell, (char *) ShellArgs, KwmCommand, NULL};
        int StatusCode = execvp(Exec[0], Exec);
        exit(StatusCode);
    }
}

void ActivateMode(const char *Mode)
{
    mode *BindingMode = GetBindingMode(Mode);
    if(BindingMode)
    {
        printf("Activate mode: %s\n", Mode);
        ActiveBindingMode = BindingMode;

        /* TODO(koekeishiya): Clean up 'Kwm' compatibility mode */
        if((Compatibility & (1 << 0)) &&
           (ActiveBindingMode->Color))
            ExecuteKwmBorderCommand();

        if(ActiveBindingMode->Prefix)
            UpdatePrefixTimer();
    }
}

mode *CreateBindingMode(const char *Mode)
{
    mode *Result = (mode *) malloc(sizeof(mode));
    memset(Result, 0, sizeof(mode));
    Result->Name = strdup(Mode);

    mode *Chain = GetLastBindingMode();
    Chain->Next = Result;
    return Result;
}

mode *GetLastBindingMode()
{
    mode *BindingMode = &DefaultBindingMode;
    while(BindingMode->Next)
        BindingMode = BindingMode->Next;

    return BindingMode;
}

mode *GetBindingMode(const char *Mode)
{
    mode *Result = NULL;

    mode *BindingMode = &DefaultBindingMode;
    while(BindingMode)
    {
        if(StringsAreEqual(BindingMode->Name, Mode))
        {
            Result = BindingMode;
            break;
        }

        BindingMode = BindingMode->Next;
    }

    return Result;
}

internal inline bool
CompareCmdKey(hotkey *A, hotkey *B)
{
    if(HasFlags(A, Hotkey_Flag_Cmd))
    {
        return (HasFlags(B, Hotkey_Flag_LCmd) ||
                HasFlags(B, Hotkey_Flag_RCmd) ||
                HasFlags(B, Hotkey_Flag_Cmd));
    }
    else
    {
        return ((HasFlags(A, Hotkey_Flag_LCmd) == HasFlags(B, Hotkey_Flag_LCmd)) &&
                (HasFlags(A, Hotkey_Flag_RCmd) == HasFlags(B, Hotkey_Flag_RCmd)) &&
                (HasFlags(A, Hotkey_Flag_Cmd) == HasFlags(B, Hotkey_Flag_Cmd)));
    }
}

internal inline bool
CompareShiftKey(hotkey *A, hotkey *B)
{
    if(HasFlags(A, Hotkey_Flag_Shift))
    {
        return (HasFlags(B, Hotkey_Flag_LShift) ||
                HasFlags(B, Hotkey_Flag_RShift) ||
                HasFlags(B, Hotkey_Flag_Shift));
    }
    else
    {
        return ((HasFlags(A, Hotkey_Flag_LShift) == HasFlags(B, Hotkey_Flag_LShift)) &&
                (HasFlags(A, Hotkey_Flag_RShift) == HasFlags(B, Hotkey_Flag_RShift)) &&
                (HasFlags(A, Hotkey_Flag_Shift) == HasFlags(B, Hotkey_Flag_Shift)));
    }
}

internal inline bool
CompareAltKey(hotkey *A, hotkey *B)
{
    if(HasFlags(A, Hotkey_Flag_Alt))
    {
        return (HasFlags(B, Hotkey_Flag_LAlt) ||
                HasFlags(B, Hotkey_Flag_RAlt) ||
                HasFlags(B, Hotkey_Flag_Alt));
    }
    else
    {
        return ((HasFlags(A, Hotkey_Flag_LAlt) == HasFlags(B, Hotkey_Flag_LAlt)) &&
                (HasFlags(A, Hotkey_Flag_RAlt) == HasFlags(B, Hotkey_Flag_RAlt)) &&
                (HasFlags(A, Hotkey_Flag_Alt) == HasFlags(B, Hotkey_Flag_Alt)));
    }
}

internal inline bool
CompareControlKey(hotkey *A, hotkey *B)
{
    if(HasFlags(A, Hotkey_Flag_Control))
    {
        return (HasFlags(B, Hotkey_Flag_LControl) ||
                HasFlags(B, Hotkey_Flag_RControl) ||
                HasFlags(B, Hotkey_Flag_Control));
    }
    else
    {
        return ((HasFlags(A, Hotkey_Flag_LControl) == HasFlags(B, Hotkey_Flag_LControl)) &&
                (HasFlags(A, Hotkey_Flag_RControl) == HasFlags(B, Hotkey_Flag_RControl)) &&
                (HasFlags(A, Hotkey_Flag_Control) == HasFlags(B, Hotkey_Flag_Control)));
    }
}

internal inline bool
HotkeysAreEqual(hotkey *A, hotkey *B)
{
    if(A && B)
    {
        return CompareCmdKey(A, B) &&
               CompareShiftKey(A, B) &&
               CompareAltKey(A, B) &&
               CompareControlKey(A, B) &&
               A->Key == B->Key;
    }

    return false;
}

internal hotkey
CreateHotkeyFromCGEvent(CGEventRef Event)
{
    CGEventFlags Flags = CGEventGetFlags(Event);
    hotkey Eventkey = {};

    if((Flags & Event_Mask_Cmd) == Event_Mask_Cmd)
    {
        if((Flags & Event_Mask_LCmd) == Event_Mask_LCmd)
            AddFlags(&Eventkey, Hotkey_Flag_LCmd);
        else if((Flags & Event_Mask_RCmd) == Event_Mask_RCmd)
            AddFlags(&Eventkey, Hotkey_Flag_RCmd);
        else
            AddFlags(&Eventkey, Hotkey_Flag_Cmd);
    }

    if((Flags & Event_Mask_Shift) == Event_Mask_Shift)
    {
        if((Flags & Event_Mask_LShift) == Event_Mask_LShift)
            AddFlags(&Eventkey, Hotkey_Flag_LShift);
        else if((Flags & Event_Mask_RShift) == Event_Mask_RShift)
            AddFlags(&Eventkey, Hotkey_Flag_RShift);
        else
            AddFlags(&Eventkey, Hotkey_Flag_Shift);
    }

    if((Flags & Event_Mask_Alt) == Event_Mask_Alt)
    {
        if((Flags & Event_Mask_LAlt) == Event_Mask_LAlt)
            AddFlags(&Eventkey, Hotkey_Flag_LAlt);
        else if((Flags & Event_Mask_RAlt) == Event_Mask_RAlt)
            AddFlags(&Eventkey, Hotkey_Flag_RAlt);
        else
            AddFlags(&Eventkey, Hotkey_Flag_Alt);
    }

    if((Flags & Event_Mask_Control) == Event_Mask_Control)
    {
        if((Flags & Event_Mask_LControl) == Event_Mask_LControl)
            AddFlags(&Eventkey, Hotkey_Flag_LControl);
        else if((Flags & Event_Mask_RControl) == Event_Mask_RControl)
            AddFlags(&Eventkey, Hotkey_Flag_RControl);
        else
            AddFlags(&Eventkey, Hotkey_Flag_Control);
    }

    Eventkey.Key = CGEventGetIntegerValueField(Event, kCGKeyboardEventKeycode);
    return Eventkey;
}

internal bool
HotkeyExists(uint32_t Flags, CGKeyCode Keycode, hotkey **Result, const char *Mode)
{
    hotkey TempHotkey = {};
    TempHotkey.Flags = Flags;
    TempHotkey.Key = Keycode;

    mode *BindingMode = GetBindingMode(Mode);
    if(BindingMode)
    {
        hotkey *Hotkey = BindingMode->Hotkey;
        while(Hotkey)
        {
            if(HotkeysAreEqual(Hotkey, &TempHotkey))
            {
                *Result = Hotkey;
                return true;
            }

            Hotkey = Hotkey->Next;
        }
    }

    return false;
}

bool HotkeyForCGEvent(CGEventRef Event, hotkey **Hotkey)
{
    hotkey Eventkey = CreateHotkeyFromCGEvent(Event);
    return HotkeyExists(Eventkey.Flags, Eventkey.Key, Hotkey, ActiveBindingMode->Name);
}

void SendKeySequence(const char *Sequence)
{
    CFStringRef SequenceRef = CFStringCreateWithCString(NULL, Sequence, kCFStringEncodingMacRoman);
    CGEventRef KeyDownEvent = CGEventCreateKeyboardEvent(NULL, 0, true);
    CGEventRef KeyUpEvent = CGEventCreateKeyboardEvent(NULL, 0, false);

    UniChar OutputBuffer;
    for(size_t Index = 0;
        *Sequence != '\0';
        ++Index, ++Sequence)
    {
        CFStringGetCharacters(SequenceRef, CFRangeMake(Index, 1), &OutputBuffer);

        CGEventSetFlags(KeyDownEvent, 0);
        CGEventKeyboardSetUnicodeString(KeyDownEvent, 1, &OutputBuffer);
        CGEventPost(kCGHIDEventTap, KeyDownEvent);

        CGEventSetFlags(KeyUpEvent, 0);
        CGEventKeyboardSetUnicodeString(KeyUpEvent, 1, &OutputBuffer);
        CGEventPost(kCGHIDEventTap, KeyUpEvent);

        usleep(100);
    }

    CFRelease(KeyUpEvent);
    CFRelease(KeyDownEvent);
    CFRelease(SequenceRef);
}

internal CGEventFlags
CreateCGEventFlagsFromHotkey(hotkey *Hotkey)
{
    CGEventFlags Flags = 0;

    if(HasFlags(Hotkey, Hotkey_Flag_Cmd))
        Flags |= Event_Mask_Cmd;
    else if(HasFlags(Hotkey, Hotkey_Flag_LCmd))
        Flags |= Event_Mask_LCmd | Event_Mask_Cmd;
    else if(HasFlags(Hotkey, Hotkey_Flag_RCmd))
        Flags |= Event_Mask_RCmd | Event_Mask_Cmd;

    if(HasFlags(Hotkey, Hotkey_Flag_Shift))
        Flags |= Event_Mask_Shift;
    else if(HasFlags(Hotkey, Hotkey_Flag_LShift))
        Flags |= Event_Mask_LShift | Event_Mask_Shift;
    else if(HasFlags(Hotkey, Hotkey_Flag_RShift))
        Flags |= Event_Mask_RShift | Event_Mask_Shift;

    if(HasFlags(Hotkey, Hotkey_Flag_Alt))
        Flags |= Event_Mask_Alt;
    else if(HasFlags(Hotkey, Hotkey_Flag_LAlt))
        Flags |= Event_Mask_LAlt | Event_Mask_Alt;
    else if(HasFlags(Hotkey, Hotkey_Flag_RAlt))
        Flags |= Event_Mask_RAlt | Event_Mask_Alt;

    if(HasFlags(Hotkey, Hotkey_Flag_Control))
        Flags |= Event_Mask_Control;
    else if(HasFlags(Hotkey, Hotkey_Flag_LControl))
        Flags |= Event_Mask_LControl | Event_Mask_Control;
    else if(HasFlags(Hotkey, Hotkey_Flag_RControl))
        Flags |= Event_Mask_RControl | Event_Mask_Control;

    return Flags;
}

void SendKeyPress(char *KeySym)
{
    hotkey Hotkey = {};
    ParseKeySym(KeySym, &Hotkey);
    CGEventFlags Flags = CreateCGEventFlagsFromHotkey(&Hotkey);

    CGEventRef KeyDownEvent = CGEventCreateKeyboardEvent(NULL, Hotkey.Key, true);
    CGEventSetFlags(KeyDownEvent, Flags);
    CGEventPost(kCGHIDEventTap, KeyDownEvent);

    CGEventRef KeyUpEvent = CGEventCreateKeyboardEvent(NULL, Hotkey.Key, false);
    CGEventSetFlags(KeyUpEvent, Flags);
    CGEventPost(kCGHIDEventTap, KeyUpEvent);

    CFRelease(KeyDownEvent);
    CFRelease(KeyUpEvent);
}
