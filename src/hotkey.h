#ifndef KHD_HOTKEY_H
#define KHD_HOTKEY_H

#include <stdint.h>
#include <time.h>
#include <Carbon/Carbon.h>

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

    Hotkey_Flag_Passthrough = (1 << 10),
};

struct hotkey;

struct mode
{
    char *Name;
    char *Color;

    bool Prefix;
    double Timeout;
    char *Restore;
    timespec Time;

    hotkey *Hotkey;
    mode *Next;
};

struct hotkey
{
    char *Mode;

    uint32_t Flags;
    CGKeyCode Key;
    char *Command;

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

void Execute(char *Command);
bool HotkeyForCGEvent(CGEventRef Event, hotkey *Hotkey);

mode *CreateBindingMode(const char *Mode);
mode *GetBindingMode(const char *Mode);
mode *GetLastBindingMode();
void ActivateMode(const char *Mode);

void SendKeySequence(const char *Sequence);
void SendKeyPress(char *KeySym);

#endif
