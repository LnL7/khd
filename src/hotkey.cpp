#include "hotkey.h"
#include "locale.h"
#include "parse.h"
#include <string.h>
#include <pthread.h>
#include <mach/mach_time.h>

#define internal static
#define local_persist static
#define CLOCK_PRECISION 1E-9

extern mode DefaultBindingMode;
extern mode *ActiveBindingMode;
extern uint32_t Compatibility;
extern char *FocusedApp;
extern pthread_mutex_t Lock;
extern hotkey Modifiers;
extern long long ModifierTriggerTime;
extern double ModifierTriggerTimeout;

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
GetTimeDiff(long long A, long long B)
{
    return (A - B) * CLOCK_PRECISION;
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

            if(GetTimeDiff(Time, ActiveBindingMode->Time) >= ActiveBindingMode->Timeout)
            {
                if(ActiveBindingMode->Restore)
                    ActivateMode(ActiveBindingMode->Restore);
                else
                    ActivateMode("default");
            }
        }
    });
}


internal void
Execute(char *Command)
{
    local_persist char Arg[] = "-c";
    local_persist char *Shell = getenv("SHELL");
    if(!Shell)
    {
        Shell = strdup("/bin/bash");
    }

    int ChildPID = fork();
    if(ChildPID == 0)
    {
        char *Exec[] = { Shell, Arg, Command, NULL};
        int StatusCode = execvp(Exec[0], Exec);
        exit(StatusCode);
    }
}

internal inline void
ExecuteKwmBorderCommand()
{
    char KwmCommand[64] = "kwmc config border focused color ";
    strcat(KwmCommand, ActiveBindingMode->Color);
    Execute(KwmCommand);
}

internal inline bool
VerifyHotkeyType(hotkey *Hotkey)
{
    if(!Hotkey->App)
        return true;

    if(Hotkey->Type == Hotkey_Include)
    {
        pthread_mutex_lock(&Lock);
        char **App = Hotkey->App;
        while(*App)
        {
            if(FocusedApp && StringsAreEqual(*App, FocusedApp))
            {
                pthread_mutex_unlock(&Lock);
                return true;
            }

            ++App;
        }

        pthread_mutex_unlock(&Lock);
        return false;
    }
    else if(Hotkey->Type == Hotkey_Exclude)
    {
        pthread_mutex_lock(&Lock);
        char **App = Hotkey->App;
        while(*App)
        {
            if(FocusedApp && StringsAreEqual(*App, FocusedApp))
            {
                pthread_mutex_unlock(&Lock);
                return false;
            }

            ++App;
        }

        pthread_mutex_unlock(&Lock);
        return true;
    }

    return true;
}

bool ExecuteHotkey(hotkey *Hotkey)
{
    bool Result = VerifyHotkeyType(Hotkey);
    if(Result)
    {
        Execute(Hotkey->Command);
        if(ActiveBindingMode->Prefix)
            UpdatePrefixTimer();
    }

    return Result;
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
CompareFnKey(hotkey *A, hotkey *B)
{
    return HasFlags(A, Hotkey_Flag_Fn) == HasFlags(B, Hotkey_Flag_Fn);
}

internal inline bool
HotkeysAreEqual(hotkey *A, hotkey *B)
{
    if(A && B)
    {
        if(HasFlags(A, Hotkey_Flag_Literal) != HasFlags(B, Hotkey_Flag_Literal))
            return false;

        if(!HasFlags(A, Hotkey_Flag_Literal) &&
           !HasFlags(B, Hotkey_Flag_Literal))
        {
            return CompareCmdKey(A, B) &&
                   CompareShiftKey(A, B) &&
                   CompareAltKey(A, B) &&
                   CompareControlKey(A, B) &&
                   CompareFnKey(A, B);
        }
        else
        {
            return CompareCmdKey(A, B) &&
                   CompareShiftKey(A, B) &&
                   CompareAltKey(A, B) &&
                   CompareControlKey(A, B) &&
                   CompareFnKey(A, B) &&
                   A->Key == B->Key;
        }
    }

    return false;
}

internal hotkey
CreateHotkeyFromCGEvent(CGEventFlags Flags, CGKeyCode Key)
{
    hotkey Eventkey = {};
    Eventkey.Key = Key;

    if((Flags & Event_Mask_Cmd) == Event_Mask_Cmd)
    {
        bool Left = (Flags & Event_Mask_LCmd) == Event_Mask_LCmd;
        bool Right = (Flags & Event_Mask_RCmd) == Event_Mask_RCmd;

        if(Left)
            AddFlags(&Eventkey, Hotkey_Flag_LCmd);

        if(Right)
            AddFlags(&Eventkey, Hotkey_Flag_RCmd);

        if(!Left && !Right)
            AddFlags(&Eventkey, Hotkey_Flag_Cmd);
    }

    if((Flags & Event_Mask_Shift) == Event_Mask_Shift)
    {
        bool Left = (Flags & Event_Mask_LShift) == Event_Mask_LShift;
        bool Right = (Flags & Event_Mask_RShift) == Event_Mask_RShift;

        if(Left)
            AddFlags(&Eventkey, Hotkey_Flag_LShift);

        if(Right)
            AddFlags(&Eventkey, Hotkey_Flag_RShift);

        if(!Left && !Right)
            AddFlags(&Eventkey, Hotkey_Flag_Shift);
    }

    if((Flags & Event_Mask_Alt) == Event_Mask_Alt)
    {
        bool Left = (Flags & Event_Mask_LAlt) == Event_Mask_LAlt;
        bool Right = (Flags & Event_Mask_RAlt) == Event_Mask_RAlt;

        if(Left)
            AddFlags(&Eventkey, Hotkey_Flag_LAlt);

        if(Right)
            AddFlags(&Eventkey, Hotkey_Flag_RAlt);

        if(!Left && !Right)
            AddFlags(&Eventkey, Hotkey_Flag_Alt);
    }

    if((Flags & Event_Mask_Control) == Event_Mask_Control)
    {
        bool Left = (Flags & Event_Mask_LControl) == Event_Mask_LControl;
        bool Right = (Flags & Event_Mask_RControl) == Event_Mask_RControl;

        if(Left)
            AddFlags(&Eventkey, Hotkey_Flag_LControl);

        if(Right)
            AddFlags(&Eventkey, Hotkey_Flag_RControl);

        if(!Left && !Right)
            AddFlags(&Eventkey, Hotkey_Flag_Control);
    }

    if((Flags & Event_Mask_Fn) == Event_Mask_Fn)
    {
        AddFlags(&Eventkey, Hotkey_Flag_Fn);
    }

    return Eventkey;
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

    if(HasFlags(Hotkey, Hotkey_Flag_Fn))
        Flags |= Event_Mask_Fn;

    return Flags;
}

internal inline void
ExecuteModifierOnlyHotkey(uint32_t Flag)
{
    CGEventFlags EventFlags = CreateCGEventFlagsFromHotkey(&Modifiers);
    hotkey *Hotkey = NULL;
    if(HotkeyForCGEvent(EventFlags, 0, &Hotkey, false))
    {
        ExecuteHotkey(Hotkey);
    }
}

internal inline void
ModifierPressed(uint32_t Flag)
{
    AddFlags(&Modifiers, Flag);
    ClockGetTime(&ModifierTriggerTime);
}

internal inline void
ModifierReleased(uint32_t Flag)
{
    long long Time;
    ClockGetTime(&Time);
    if(GetTimeDiff(Time, ModifierTriggerTime) < ModifierTriggerTimeout)
    {
        ExecuteModifierOnlyHotkey(Flag);
    }

    ClearFlags(&Modifiers, Flag);
}

void RefreshModifierState(CGEventFlags Flags, CGKeyCode Key)
{
    if(Key == Modifier_Keycode_Left_Cmd)
    {
        if(Flags & Event_Mask_LCmd)
            ModifierPressed(Hotkey_Flag_LCmd);
        else
            ModifierReleased(Hotkey_Flag_LCmd);
    }
    else if(Key == Modifier_Keycode_Right_Cmd)
    {
        if(Flags & Event_Mask_RCmd)
            ModifierPressed(Hotkey_Flag_RCmd);
        else
            ModifierReleased(Hotkey_Flag_RCmd);
    }
    else if(Key == Modifier_Keycode_Left_Shift)
    {
        if(Flags & Event_Mask_LShift)
            ModifierPressed(Hotkey_Flag_LShift);
        else
            ModifierReleased(Hotkey_Flag_LShift);
    }
    else if(Key == Modifier_Keycode_Right_Shift)
    {
        if(Flags & Event_Mask_RShift)
            ModifierPressed(Hotkey_Flag_RShift);
        else
            ModifierReleased(Hotkey_Flag_RShift);
    }
    else if(Key == Modifier_Keycode_Left_Alt)
    {
        if(Flags & Event_Mask_LAlt)
            ModifierPressed(Hotkey_Flag_LAlt);
        else
            ModifierReleased(Hotkey_Flag_LAlt);
    }
    else if(Key == Modifier_Keycode_Right_Alt)
    {
        if(Flags & Event_Mask_RAlt)
            ModifierPressed(Hotkey_Flag_RAlt);
        else
            ModifierReleased(Hotkey_Flag_RAlt);
    }
    else if(Key == Modifier_Keycode_Left_Ctrl)
    {
        if(Flags & Event_Mask_LControl)
            ModifierPressed(Hotkey_Flag_LControl);
        else
            ModifierReleased(Hotkey_Flag_LControl);
    }
    else if(Key == Modifier_Keycode_Right_Ctrl)
    {
        if(Flags & Event_Mask_RControl)
            ModifierPressed(Hotkey_Flag_RControl);
        else
            ModifierReleased(Hotkey_Flag_RControl);
    }
    else if(Key == Modifier_Keycode_Fn)
    {
        if(Flags & Event_Mask_Fn)
            ModifierPressed(Hotkey_Flag_Fn);
        else
            ModifierReleased(Hotkey_Flag_Fn);
    }
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

bool HotkeyForCGEvent(CGEventFlags Flags, CGKeyCode Key, hotkey **Hotkey, bool Literal)
{
    hotkey Eventkey = CreateHotkeyFromCGEvent(Flags, Key);
    if(Literal)
    {
        AddFlags(&Eventkey, Hotkey_Flag_Literal);
    }

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
