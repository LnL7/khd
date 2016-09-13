#include "parse.h"
#include "tokenize.h"
#include "hotkey.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define internal static
extern mode DefaultBindingMode;
extern mode *ActiveBindingMode;

/*
void ParseHotkeyModifiers(hotkey *Hotkey, std::string KeySym)
{
    std::vector<std::string> Modifiers = SplitString(KeySym, '+');

    for(size_t Index = 0; Index < Modifiers.size(); ++Index)
    {
        if(Modifiers[ModIndex] == "cmd")
            AddFlags(Hotkey, Hotkey_Modifier_Flag_Cmd);
        else if(Modifiers[ModIndex] == "lcmd")
            AddFlags(Hotkey, Hotkey_Modifier_Flag_LCmd);
        else if(Modifiers[ModIndex] == "rcmd")
            AddFlags(Hotkey, Hotkey_Modifier_Flag_RCmd);
        else if(Modifiers[ModIndex] == "alt")
            AddFlags(Hotkey, Hotkey_Modifier_Flag_Alt);
        else if(Modifiers[ModIndex] == "lalt")
            AddFlags(Hotkey, Hotkey_Modifier_Flag_LAlt);
        else if(Modifiers[ModIndex] == "ralt")
            AddFlags(Hotkey, Hotkey_Modifier_Flag_RAlt);
        else if(Modifiers[ModIndex] == "shift")
            AddFlags(Hotkey, Hotkey_Modifier_Flag_Shift);
        else if(Modifiers[ModIndex] == "lshift")
            AddFlags(Hotkey, Hotkey_Modifier_Flag_LShift);
        else if(Modifiers[ModIndex] == "rshift")
            AddFlags(Hotkey, Hotkey_Modifier_Flag_RShift);
        else if(Modifiers[ModIndex] == "ctrl")
            AddFlags(Hotkey, Hotkey_Modifier_Flag_Control);
        else
            Hotkey->Mode = Modifiers[ModIndex];
    }
}

bool ParseHotkey(std::string KeySym, std::string Command, hotkey *Hotkey, bool Passthrough, bool KeycodeInHex)
{
    std::vector<std::string> KeyTokens = SplitString(KeySym, '-');
    if(KeyTokens.size() != 2)
        return false;

    Hotkey->Mode = "default";
    KwmParseHotkeyModifiers(Hotkey, KeyTokens[0]);
    DetermineHotkeyState(Hotkey, Command);
    Hotkey->Command = Command;
    if(Passthrough)
        AddFlags(Hotkey, Hotkey_Modifier_Flag_Passthrough);

    CGKeyCode Keycode;
    bool Result = false;
    if(KeycodeInHex)
    {
        Result = true;
        Keycode = ConvertHexStringToInt(KeyTokens[1]);
        DEBUG("bindcode: " << Keycode);
    }
    else
    {
        Result = LayoutIndependentKeycode(KeyTokens[1], &Keycode);
        if(!Result)
            Result = KeycodeFromChar(KeyTokens[1][0], &Keycode);
    }

    Hotkey->Key = Keycode;
    return Result;
}
*/

internal void
Error(const char *Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    vfprintf(stderr, Format, Args);
    va_end(Args);
}

internal void
ParseCommand(tokenizer *Tokenizer, void *Hotkey)
{
    token Command = GetToken(Tokenizer);
    switch(Command.Type)
    {
        case Token_Command:
        {
            if(RequireToken(Tokenizer, Token_CloseBrace))
            {
                // TODO(koekeishiya): Apply command to bind.
                printf("Token_Command: %.*s\n", Command.Length, Command.Text);
            }
            else
            {
                Error("Expected 'Token_CloseBrace' to end a command!\n");
            }
        } break;
        default:
        {
            Error("Expected token of type 'Token_Command' after symbols!\n");
        } break;
    }
}

// TODO(koekeishiya): Create new hotkey from identifier token
internal void
ParseIdentifier(token *Token, hotkey *Hotkey)
{
    printf("Token_Identifier: %.*s\n", Token->Length, Token->Text);

    char *At = Token->Text;
    bool ReachedKeyDelim = false;

    for(int Index = 0;
        Index < Token->Length;
        ++Index, ++Token->Text)
    {
        if((Token->Text[0] == '+') ||
           (Token->Text[0] == '-') ||
           (Index == Token->Length - 1))
        {
            char *Mod = At;
            int Length = Token->Text - At;

            if(!ReachedKeyDelim)
                printf("Token_Modifier: %.*s\n", Length, Mod);
            else
                printf("Token_Key: %.*s\n", Length, Mod);

            if(Token->Text[0] == '-')
                ReachedKeyDelim = true;

            ++Token->Text;
            At = Token->Text;
        }
        else
        {
            // NOTE(koekeishiya): Not a delimeter, continue
        }
    }
}

/*
struct mode
{
    char *Name;
    uint32_t Color;

    bool Prefix;
    double Timeout;
    char *Restore;

    hotkey *Hotkey;
    mode *Next;
};

struct hotkey
{
    mode *Mode;

    uint32_t Flags;
    CGKeyCode Key;
    char *Command;

    hotkey *Next;
};
 * */

// TODO(koekeishiya): We want to clear existing config information before reloading.
// The active binding mode should also be pointing to the 'default' mode.
void ParseConfig(char *Contents)
{
    hotkey *Hotkey = (hotkey *) malloc(sizeof(hotkey));
    memset(Hotkey, 0, sizeof(hotkey));
    ActiveBindingMode->Hotkey = Hotkey;

    tokenizer Tokenizer = { Contents };
    bool Parsing = true;
    while(Parsing)
    {
        token Token = GetToken(&Tokenizer);
        switch(Token.Type)
        {
            case Token_EndOfStream:
            {
                Parsing = false;
            } break;
            case Token_Comment:
            {
                printf("Token_Comment: %.*s\n", Token.Length, Token.Text);
            } break;
            case Token_Identifier:
            {
                ParseIdentifier(&Token, Hotkey);
                ParseCommand(&Tokenizer, NULL);
            } break;
            default:
            {
                printf("%d: %.*s\n", Token.Type, Token.Length, Token.Text);
            } break;
        }
    }

    free(Contents);
}
