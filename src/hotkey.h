#ifndef KHD_HOTKEY_H
#define KHD_HOTKEY_H

#include <stdint.h>
#include <Carbon/Carbon.h>

#define Modifier_Keycode_Left_Cmd     0x37
#define Modifier_Keycode_Right_Cmd    0x36
#define Modifier_Keycode_Left_Shift   0x38
#define Modifier_Keycode_Right_Shift  0x3C
#define Modifier_Keycode_Left_Alt     0x3A
#define Modifier_Keycode_Right_Alt    0x3D
#define Modifier_Keycode_Left_Ctrl    0x3B
#define Modifier_Keycode_Right_Ctrl   0x3E
#define Modifier_Keycode_Fn           0x3F

enum osx_event_mask
{
    Event_Mask_Alt = 0x00080000,
    Event_Mask_LAlt = 0x00000020,
    Event_Mask_RAlt = 0x00000040,

    Event_Mask_Shift = 0x00020000,
    Event_Mask_LShift = 0x00000002,
    Event_Mask_RShift = 0x00000004,

    Event_Mask_Cmd = 0x00100000,
    Event_Mask_LCmd = 0x00000008,
    Event_Mask_RCmd = 0x00000010,

    Event_Mask_Control = 0x00040000,
    Event_Mask_LControl = 0x00000001,
    Event_Mask_RControl = 0x00002000,

    Event_Mask_Fn = kCGEventFlagMaskSecondaryFn,
};

enum hotkey_flag
{
    Hotkey_Flag_Alt = (1 << 0),
    Hotkey_Flag_LAlt = (1 << 1),
    Hotkey_Flag_RAlt = (1 << 2),

    Hotkey_Flag_Shift = (1 << 3),
    Hotkey_Flag_LShift = (1 << 4),
    Hotkey_Flag_RShift = (1 << 5),

    Hotkey_Flag_Cmd = (1 << 6),
    Hotkey_Flag_LCmd = (1 << 7),
    Hotkey_Flag_RCmd = (1 << 8),

    Hotkey_Flag_Control = (1 << 9),
    Hotkey_Flag_LControl = (1 << 10),
    Hotkey_Flag_RControl = (1 << 11),

    Hotkey_Flag_Fn = (1 << 12),

    Hotkey_Flag_Passthrough = (1 << 13),
    Hotkey_Flag_Literal = (1 << 14),
};

enum hotkey_type
{
    Hotkey_Default,
    Hotkey_Include,
    Hotkey_Exclude,
};

struct hotkey;

struct mode
{
    char *Name;
    char *Color;

    bool Prefix;
    double Timeout;
    char *Restore;
    long long Time;

    hotkey *Hotkey;
    mode *Next;
};

struct hotkey
{
    char *Mode;

    hotkey_type Type;
    uint32_t Flags;
    CGKeyCode Key;
    char *Command;
    char **App;

    hotkey *Next;
};

inline void
AddFlags(hotkey *Hotkey, uint32_t Flag)
{
    Hotkey->Flags |= Flag;
}

inline bool
HasFlags(hotkey *Hotkey, uint32_t Flag)
{
    bool Result = Hotkey->Flags & Flag;
    return Result;
}

inline void
ClearFlags(hotkey *Hotkey, uint32_t Flag)
{
    Hotkey->Flags &= ~Flag;
}

bool ExecuteHotkey(hotkey *Hotkey);
bool HotkeyForCGEvent(CGEventFlags Flags, CGKeyCode Key, hotkey **Hotkey, bool Literal);
void RefreshModifierState(CGEventFlags Flags, CGKeyCode Key);

mode *CreateBindingMode(const char *Mode);
mode *GetBindingMode(const char *Mode);
mode *GetLastBindingMode();
void ActivateMode(const char *Mode);

void SendKeySequence(const char *Sequence);
void SendKeyPress(char *KeySym);

#endif
