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
    exit(EXIT_FAILURE);
}

internal inline unsigned int
HexToInt(char *Hex)
{
    uint32_t Result;
    sscanf(Hex, "%x", &Result);
    return Result;
}

internal char *
AllocAndCopyString(char *Text, int Length)
{
    char *Result = (char *) malloc(Length + 1);
    strncpy(Result, Text, Length);
    Result[Length] = '\0';

    return Result;
}

internal inline hotkey *
AllocHotkey()
{
    hotkey *Hotkey = (hotkey *) malloc(sizeof(hotkey));
    memset(Hotkey, 0, sizeof(hotkey));
    Hotkey->Mode = strdup("default");

    return Hotkey;
}

internal inline void
AppendHotkey(hotkey *Hotkey)
{
    mode *BindingMode = GetBindingMode(Hotkey->Mode);
    if(!BindingMode)
        BindingMode = CreateBindingMode(Hotkey->Mode);

    if(BindingMode->Hotkey)
    {
        hotkey *Last = BindingMode->Hotkey;
        while(Last->Next)
            Last = Last->Next;

        Last->Next = Hotkey;
    }
    else
    {
        BindingMode->Hotkey = Hotkey;
    }
}

internal void
StripTrailingWhiteSpace(token *Token)
{
    while(IsWhiteSpace(Token->Text[Token->Length-1]))
        --Token->Length;
}

internal void
ParseCommand(tokenizer *Tokenizer, hotkey *Hotkey)
{
    token Command = GetToken(Tokenizer);
    switch(Command.Type)
    {
        case Token_Command:
        {
            if(RequireToken(Tokenizer, Token_CloseBrace))
            {
                StripTrailingWhiteSpace(&Command);
                Hotkey->Command = AllocAndCopyString(Command.Text, Command.Length);
                AppendHotkey(Hotkey);
                printf("Token_Command: %s\n", Hotkey->Command);
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

void AddHotkeyModifier(char *Mod, int Length, hotkey *Hotkey)
{
    char *Modifier = AllocAndCopyString(Mod, Length);
    printf("Token_Modifier: %s\n", Modifier);

    if(strcmp(Modifier, "cmd") == 0)
    {
        AddFlags(Hotkey, Hotkey_Flag_Cmd);
    }
    else if(strcmp(Modifier, "lcmd") == 0)
    {
        AddFlags(Hotkey, Hotkey_Flag_LCmd);
    }
    else if(strcmp(Modifier, "rcmd") == 0)
    {
        AddFlags(Hotkey, Hotkey_Flag_RCmd);
    }
    else if(strcmp(Modifier, "alt") == 0)
    {
        AddFlags(Hotkey, Hotkey_Flag_Alt);
    }
    else if(strcmp(Modifier, "lalt") == 0)
    {
        AddFlags(Hotkey, Hotkey_Flag_LAlt);
    }
    else if(strcmp(Modifier, "ralt") == 0)
    {
        AddFlags(Hotkey, Hotkey_Flag_RAlt);
    }
    else if(strcmp(Modifier, "shift") == 0)
    {
        AddFlags(Hotkey, Hotkey_Flag_Shift);
    }
    else if(strcmp(Modifier, "lshift") == 0)
    {
        AddFlags(Hotkey, Hotkey_Flag_LShift);
    }
    else if(strcmp(Modifier, "rshift") == 0)
    {
        AddFlags(Hotkey, Hotkey_Flag_RShift);
    }
    else if(strcmp(Modifier, "ctrl") == 0)
    {
        AddFlags(Hotkey, Hotkey_Flag_Control);
    }
    else
    {
        free(Hotkey->Mode);
        Hotkey->Mode = strdup(Modifier);
    }

    free(Modifier);
}

internal bool
ParseIdentifier(token *Token, hotkey *Hotkey)
{
    bool Result = true;

    int At = 0;
    bool KeyDelim = false;
    token Key = {};

    for(int Index = 0;
        Index < Token->Length;
        ++Index)
    {
        char C = Token->Text[Index];
        switch(C)
        {
            case '+':
            case '-':
            {
                char *Mod = Token->Text + At;
                int Length = Index - At;

                if(Length > 0)
                {
                    AddHotkeyModifier(Mod, Length, Hotkey);
                }

                if(C == '-')
                    KeyDelim = true;

                At = Index + 1;
            } break;
            default:
            {
                if(KeyDelim && Index == Token->Length - 1)
                {
                    Key.Length = Index - At + 1;
                    Key.Text = AllocAndCopyString(Token->Text + At, Key.Length);
                    printf("Token_Key: %s\n", Key.Text);
                }
            } break;
        }
    }

    if(Key.Length > 1)
    {
        if(1)
        {
            Hotkey->Key = HexToInt(Key.Text);
        }
        else
        {
            Result = LayoutIndependentKeycode(Key.Text, Hotkey);
        }
    }
    else if(Key.Length == 1)
    {
        Result = KeycodeFromChar(Key.Text[0], Hotkey);
    }
    else
    {
        printf("No key specified!\n");
        Result = false;
    }

    return Result;
}

// TODO(koekeishiya): We want to clear existing config information before reloading.
// The active binding mode should also be pointing to the 'default' mode.
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
                hotkey *Hotkey = AllocHotkey();
                if(ParseIdentifier(&Token, Hotkey))
                {
                    ParseCommand(&Tokenizer, Hotkey);
                }
                else
                {
                    Error("Invalid format for identifier: %.*s\n", Token.Length, Token.Text);
                }
            } break;
            default:
            {
                printf("%d: %.*s\n", Token.Type, Token.Length, Token.Text);
            } break;
        }
    }

    free(Contents);
}
