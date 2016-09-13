#include "hotkey.h"
#include <string.h>

#define internal static
#define local_persist static

extern mode DefaultBindingMode;
extern mode *ActiveBindingMode;

internal const char *Shell = "/bin/bash";
internal const char *ShellArgs = "-c";

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

internal void
Execute(char *Command)
{
    int ChildPID = fork();
    if(ChildPID == 0)
    {
        char *Exec[] = { (char *) Shell, (char *) ShellArgs, Command, NULL};
        int StatusCode = execvp(Exec[0], Exec);
        exit(StatusCode);
    }
}

internal void
ActivateMode(char *Mode)
{
    int Length = strlen(Mode);

    char *End = Mode + strlen(Mode) - 1;
    while(End > Mode && !isspace(*End))
        --End;

    mode *BindingMode = GetBindingMode(++End);
    if(BindingMode)
    {
        printf("Activate mode: %s\n", Mode);
        ActiveBindingMode = BindingMode;
    }
}

internal inline bool
StringPrefix(const char *String, const char *Prefix)
{
    size_t PrefixLength = strlen(Prefix);
    size_t StringLength = strlen(String);

    if (StringLength < PrefixLength)
        return false;
    else
        return strncmp(Prefix, String, PrefixLength) == 0;
}

void ExecuteHotkey(hotkey *Hotkey)
{
    if(Hotkey->Command)
    {
        if(StringPrefix(Hotkey->Command, "khd mode activate"))
            ActivateMode(Hotkey->Command);
        else
            Execute(Hotkey->Command);

        /*
        if(ActiveMode->Prefix)
        {
            ActiveMode->Time = std::chrono::steady_clock::now();
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, ActiveMode->Timeout * NSEC_PER_SEC), dispatch_get_main_queue(),
            ^{
                CheckPrefixTimeout();
            });
        }
        */
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

bool HotkeyForCGEvent(CGEventRef Event, hotkey *Hotkey)
{
    hotkey Eventkey = CreateHotkeyFromCGEvent(Event);
    return HotkeyExists(Eventkey.Flags, Eventkey.Key, Hotkey, ActiveBindingMode->Name);
}

internal CFStringRef
CFStringFromKeycode(CGKeyCode Keycode)
{
    TISInputSourceRef Keyboard = TISCopyCurrentASCIICapableKeyboardLayoutInputSource();
    CFDataRef Uchr = (CFDataRef) TISGetInputSourceProperty(Keyboard, kTISPropertyUnicodeKeyLayoutData);
    CFRelease(Keyboard);

    UCKeyboardLayout *KeyboardLayout = (UCKeyboardLayout *) CFDataGetBytePtr(Uchr);
    if(KeyboardLayout)
    {
        UInt32 DeadKeyState = 0;
        UniCharCount MaxStringLength = 255;
        UniCharCount ActualStringLength = 0;
        UniChar UnicodeString[MaxStringLength];

        OSStatus Status = UCKeyTranslate(KeyboardLayout, Keycode,
                                         kUCKeyActionDown, 0,
                                         LMGetKbdType(), 0,
                                         &DeadKeyState,
                                         MaxStringLength,
                                         &ActualStringLength,
                                         UnicodeString);

        if(ActualStringLength == 0 && DeadKeyState)
        {
            Status = UCKeyTranslate(KeyboardLayout, kVK_Space,
                                    kUCKeyActionDown, 0,
                                    LMGetKbdType(), 0,
                                    &DeadKeyState,
                                    MaxStringLength,
                                    &ActualStringLength,
                                    UnicodeString);
        }

        if(ActualStringLength > 0 && Status == noErr)
            return CFStringCreateWithCharacters(NULL, UnicodeString, ActualStringLength);
    }

    return NULL;
}

bool KeycodeFromChar(char Key, hotkey *Hotkey)
{
    local_persist CFMutableDictionaryRef CharToCodeDict = NULL;

    bool Result = true;
    UniChar Character = Key;
    CFStringRef CharStr;

    if(!CharToCodeDict)
    {
        CharToCodeDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 128,
                                                   &kCFCopyStringDictionaryKeyCallBacks, NULL);
        if(!CharToCodeDict)
            return false;

        for(size_t KeyIndex = 0; KeyIndex < 128; ++KeyIndex)
        {
            CFStringRef KeyString = CFStringFromKeycode((CGKeyCode)KeyIndex);
            if(KeyString != NULL)
            {
                CFDictionaryAddValue(CharToCodeDict, KeyString, (const void *)KeyIndex);
                CFRelease(KeyString);
            }
        }
    }

    CharStr = CFStringCreateWithCharacters(kCFAllocatorDefault, &Character, 1);
    if(!CFDictionaryGetValueIfPresent(CharToCodeDict, CharStr, (const void **)&Hotkey->Key))
        Result = false;

    CFRelease(CharStr);
    return Result;
}

bool LayoutIndependentKeycode(char *Key, hotkey *Hotkey)
{
    bool Result = true;

    if(StringsAreEqual(Key, "return"))
        Hotkey->Key = kVK_Return;
    else if(StringsAreEqual(Key, "tab"))
        Hotkey->Key = kVK_Tab;
    else if(StringsAreEqual(Key, "space"))
        Hotkey->Key = kVK_Space;
    else if(StringsAreEqual(Key, "backspace"))
        Hotkey->Key = kVK_Delete;
    else if(StringsAreEqual(Key, "delete"))
        Hotkey->Key = kVK_ForwardDelete;
    else if(StringsAreEqual(Key, "escape"))
        Hotkey->Key =  kVK_Escape;
    else if(StringsAreEqual(Key, "left"))
        Hotkey->Key =  kVK_LeftArrow;
    else if(StringsAreEqual(Key, "right"))
        Hotkey->Key =  kVK_RightArrow;
    else if(StringsAreEqual(Key, "up"))
        Hotkey->Key = kVK_UpArrow;
    else if(StringsAreEqual(Key, "down"))
        Hotkey->Key = kVK_DownArrow;
    else if(StringsAreEqual(Key, "f1"))
        Hotkey->Key = kVK_F1;
    else if(StringsAreEqual(Key, "f2"))
        Hotkey->Key = kVK_F2;
    else if(StringsAreEqual(Key, "f3"))
        Hotkey->Key = kVK_F3;
    else if(StringsAreEqual(Key, "f4"))
        Hotkey->Key = kVK_F4;
    else if(StringsAreEqual(Key, "f5"))
        Hotkey->Key = kVK_F5;
    else if(StringsAreEqual(Key, "f6"))
        Hotkey->Key = kVK_F6;
    else if(StringsAreEqual(Key, "f7"))
        Hotkey->Key = kVK_F7;
    else if(StringsAreEqual(Key, "f8"))
        Hotkey->Key = kVK_F8;
    else if(StringsAreEqual(Key, "f9"))
        Hotkey->Key = kVK_F9;
    else if(StringsAreEqual(Key, "f10"))
        Hotkey->Key = kVK_F10;
    else if(StringsAreEqual(Key, "f11"))
        Hotkey->Key = kVK_F11;
    else if(StringsAreEqual(Key, "f12"))
        Hotkey->Key = kVK_F12;
    else if(StringsAreEqual(Key, "f13"))
        Hotkey->Key = kVK_F13;
    else if(StringsAreEqual(Key, "f14"))
        Hotkey->Key = kVK_F14;
    else if(StringsAreEqual(Key, "f15"))
        Hotkey->Key = kVK_F15;
    else if(StringsAreEqual(Key, "f16"))
        Hotkey->Key = kVK_F16;
    else if(StringsAreEqual(Key, "f17"))
        Hotkey->Key = kVK_F17;
    else if(StringsAreEqual(Key, "f18"))
        Hotkey->Key = kVK_F18;
    else if(StringsAreEqual(Key, "f19"))
        Hotkey->Key = kVK_F19;
    else if(StringsAreEqual(Key, "f20"))
        Hotkey->Key = kVK_F20;
    else
        Result = false;

    return Result;
}

bool StringsAreEqual(const char *A, const char *B)
{
    return strcmp(A, B) == 0;
}
