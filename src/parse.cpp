#include "parse.h"
#include "tokenize.h"
#include "locale.h"
#include "hotkey.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define internal static
extern mode DefaultBindingMode;
extern mode *ActiveBindingMode;
extern uint32_t Compatibility;

/*
    if(Passthrough)
        AddFlags(Hotkey, Hotkey_Flag_Passthrough);
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

internal void
Notice(const char *Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    vfprintf(stderr, Format, Args);
    va_end(Args);
}

internal inline unsigned int
HexToInt(char *Hex)
{
    uint32_t Result;
    sscanf(Hex, "%x", &Result);
    return Result;
}

internal inline double
StringToDouble(char *String)
{
    double Result;
    sscanf(String, "%lf", &Result);
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

internal inline void
SetHotkeyMode(hotkey *Hotkey, char *Modifier)
{
    if(Hotkey->Mode)
        free(Hotkey->Mode);

    Hotkey->Mode = strdup(Modifier);
}

internal void
AddHotkeyModifier(char *Mod, int Length, hotkey *Hotkey)
{
    char *Modifier = AllocAndCopyString(Mod, Length);
    printf("Token_Modifier: %s\n", Modifier);

    if(StringsAreEqual(Modifier, "cmd"))
        AddFlags(Hotkey, Hotkey_Flag_Cmd);
    else if(StringsAreEqual(Modifier, "lcmd"))
        AddFlags(Hotkey, Hotkey_Flag_LCmd);
    else if(StringsAreEqual(Modifier, "rcmd"))
        AddFlags(Hotkey, Hotkey_Flag_RCmd);
    else if(StringsAreEqual(Modifier, "alt"))
        AddFlags(Hotkey, Hotkey_Flag_Alt);
    else if(StringsAreEqual(Modifier, "lalt"))
        AddFlags(Hotkey, Hotkey_Flag_LAlt);
    else if(StringsAreEqual(Modifier, "ralt"))
        AddFlags(Hotkey, Hotkey_Flag_RAlt);
    else if(StringsAreEqual(Modifier, "shift"))
        AddFlags(Hotkey, Hotkey_Flag_Shift);
    else if(StringsAreEqual(Modifier, "lshift"))
        AddFlags(Hotkey, Hotkey_Flag_LShift);
    else if(StringsAreEqual(Modifier, "rshift"))
        AddFlags(Hotkey, Hotkey_Flag_RShift);
    else if(StringsAreEqual(Modifier, "ctrl"))
        AddFlags(Hotkey, Hotkey_Flag_Control);
    else
        SetHotkeyMode(Hotkey, Modifier);

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
        /* TODO(koekeishiya): Check if this is a hexadecimal input
         * or a special key such as 'return'. */
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

internal void
ParseKhdModeActivate(tokenizer *Tokenizer)
{
    token TokenMode = GetToken(Tokenizer);
    char *Mode = AllocAndCopyString(TokenMode.Text, TokenMode.Length);
    ActivateMode(Mode);

    free(Mode);
}

internal void
ParseKhdModeProperties(token *TokenMode, tokenizer *Tokenizer)
{
    char *Mode = AllocAndCopyString(TokenMode->Text, TokenMode->Length);
    mode *BindingMode = GetBindingMode(Mode);
    if(!BindingMode)
        BindingMode = CreateBindingMode(Mode);

    token Token = GetToken(Tokenizer);
    if(TokenEquals(Token, "prefix"))
    {
        token Token = GetToken(Tokenizer);
        if(TokenEquals(Token, "on"))
            BindingMode->Prefix = true;
        else if(TokenEquals(Token, "off"))
            BindingMode->Prefix = false;

        printf("Prefix State: %d\n", BindingMode->Prefix);
    }
    else if(TokenEquals(Token, "timeout"))
    {
        token Token = GetToken(Tokenizer);
        switch(Token.Type)
        {
            case Token_Digit:
            {
                char *Temp = AllocAndCopyString(Token.Text, Token.Length);
                BindingMode->Timeout = StringToDouble(Temp);
                printf("Prefix Timeout: %f\n", BindingMode->Timeout);
                free(Temp);
            } break;
            default:
            {
                Notice("Expected token of type 'Token_Digit': %.*s\n", Token.Length, Token.Text);
            };
        }
    }
    else if(TokenEquals(Token, "color"))
    {
        token Token = GetToken(Tokenizer);
        BindingMode->Color = AllocAndCopyString(Token.Text, Token.Length);
        printf("Prefix Color: %s\n", BindingMode->Color);
    }
    else if(TokenEquals(Token, "restore"))
    {
        token Token = GetToken(Tokenizer);
        BindingMode->Restore = AllocAndCopyString(Token.Text, Token.Length);
        printf("Prefix Restore: %s\n", BindingMode->Restore);
    }

    free(Mode);
}

internal void
ParseKhdKwmCompatibility(tokenizer *Tokenizer)
{
    token Token = GetToken(Tokenizer);
    if(TokenEquals(Token, "on"))
    {
        Compatibility |= (1 << 0);
    }
    else if(TokenEquals(Token, "off"))
    {
        Compatibility &= ~(1 << 0);
    }
    else
    {
        Notice("Unexpected token '%.*s'\n", Token.Length, Token.Text);
    }
}

internal void
ParseKhdMode(tokenizer *Tokenizer)
{
    token Token = GetToken(Tokenizer);
    switch(Token.Type)
    {
        case Token_EndOfStream:
        {
            Notice("Unexpected end of stream when parsing khd command!\n");
        } break;
        case Token_Identifier:
        {
            if(TokenEquals(Token, "activate"))
                ParseKhdModeActivate(Tokenizer);
            else
                ParseKhdModeProperties(&Token, Tokenizer);
        } break;
        default:
        {
            Notice("Unexpected token '%.*s'\n", Token.Length, Token.Text);
        } break;
    }
}

internal void
ParseKhd(tokenizer *Tokenizer)
{
    token Token = GetToken(Tokenizer);
    switch(Token.Type)
    {
        case Token_EndOfStream:
        {
            Notice("Unexpected end of stream when parsing khd command!\n");
        } break;
        case Token_Identifier:
        {
            if(TokenEquals(Token, "reload"))
            {
                // TODO(koekeishiya): Reload config file.
            }
            else if(TokenEquals(Token, "kwm"))
            {
                ParseKhdKwmCompatibility(Tokenizer);
            }
            else if(TokenEquals(Token, "mode"))
            {
                ParseKhdMode(Tokenizer);
            }
        } break;
        default:
        {
            Notice("Unexpected token '%.*s'\n", Token.Length, Token.Text);
        } break;
    }
}

void ParseKhd(char *Contents)
{
    tokenizer Tokenizer = { Contents };
    ParseKhd(&Tokenizer);
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
                if(TokenEquals(Token, "khd"))
                {
                    ParseKhd(&Tokenizer);
                }
                else
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
