#include "parse.h"
#include "tokenizer.h"
#include "hotkey.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define internal static

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

void ParseCommand(tokenizer *Tokenizer, void *Hotkey)
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

void ParseConfig(char *Contents)
{
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
                printf("Token_Identifier: %.*s\n", Token.Length, Token.Text);
                // TODO(koekeishiya): Create new hotkey with identifier
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
