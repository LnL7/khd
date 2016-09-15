#include "hotkey.h"
#include "locale.h"
#include <string.h>

#define internal static
extern mode DefaultBindingMode;
extern mode *ActiveBindingMode;
extern uint32_t Compatibility;

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
}

mode *CreateBindingMode(char *Mode)
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

mode *GetBindingMode(char *Mode)
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

void ActivateMode(char *Mode)
{
    mode *BindingMode = GetBindingMode(Mode);
    if(BindingMode)
    {
        printf("Activate mode: %s\n", Mode);
        ActiveBindingMode = BindingMode;

        /* TODO(koekeishiya): We need some sort of 'Kwm' compatibility mode
         * to automatically issue 'kwmc config border focused color Mode->Color' */
        if((Compatibility & (1 << 0)) &&
           (ActiveBindingMode->Color))
        {
            char KwmCommand[64] = "kwmc config border focused color ";
            strcat(KwmCommand, ActiveBindingMode->Color);
            Execute(KwmCommand);
        }

        /*
        if(ActiveBindingMode->Prefix)
        {
            ActiveBindingMode->Time = std::chrono::steady_clock::now();
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, BindingMode->Timeout * NSEC_PER_SEC), dispatch_get_main_queue(),
            ^{
                CheckPrefixTimeout();
            });
        }
        */
    }
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
    return (HasFlags(A, Hotkey_Flag_Control) == HasFlags(B, Hotkey_Flag_Control));
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
        AddFlags(&Eventkey, Hotkey_Flag_Control);

    Eventkey.Key = (CGKeyCode)CGEventGetIntegerValueField(Event, kCGKeyboardEventKeycode);
    return Eventkey;
}

internal bool
HotkeyExists(uint32_t Flags, CGKeyCode Keycode, hotkey *Result, char *Mode)
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
                if(Result)
                    *Result = *Hotkey;

                return true;
            }

            Hotkey = Hotkey->Next;
        }
    }

    return false;
}

bool HotkeyForCGEvent(CGEventRef Event, hotkey *Hotkey)
{
    hotkey Eventkey = CreateHotkeyFromCGEvent(Event);
    return HotkeyExists(Eventkey.Flags, Eventkey.Key, Hotkey, ActiveBindingMode->Name);
}
