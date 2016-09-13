#include "tokenize.h"
#define internal static

internal inline bool
IsEndOfLine(char C)
{
    bool Result = ((C == '\n') ||
                   (C == '\r'));

    return Result;
}

internal inline bool
IsWhiteSpace(char C)
{
    bool Result = ((C == ' ') ||
                   (C == '\t') ||
                   IsEndOfLine(C));

    return Result;
}

internal inline bool
IsAlpha(char C)
{
    bool Result = (((C >= 'a') && (C <= 'z')) ||
                   ((C >= 'A') && (C <= 'Z')));

    return Result;
}

internal inline bool
IsNumeric(char C)
{
    bool Result = ((C >= '0') && (C <= '9'));
    return Result;
}

internal inline bool
IsHexadecimal(char C)
{
    bool Result = (((C >= 'a') && (C <= 'f')) ||
                   ((C >= 'A') && (C <= 'F')) ||
                   (IsNumeric(C)));
    return Result;
}

internal inline void
EatAllWhiteSpace(tokenizer *Tokenizer)
{
    while(Tokenizer->At[0])
    {
        if(IsWhiteSpace(Tokenizer->At[0]))
            ++Tokenizer->At;
        else
            break;
    }
}

bool RequireToken(tokenizer *Tokenizer, token_type DesiredType)
{
    token Token = GetToken(Tokenizer);
    bool Result = Token.Type == DesiredType;
    return Result;
}

bool TokenEquals(token Token, const char *Match)
{
    const char *At = Match;
    for(int Index = 0; Index < Token.Length; ++Index, ++At)
    {
        if((*At == 0) || (Token.Text[Index] != *At))
            return false;
    }

    bool Result = (*At == 0);
    return Result;
}

token GetToken(tokenizer *Tokenizer)
{
    EatAllWhiteSpace(Tokenizer);

    token Token = {};
    Token.Length = 1;
    Token.Text = Tokenizer->At;

    char C = Tokenizer->At[0];
    ++Tokenizer->At;

    switch(C)
    {
        case '\0': { Token.Type = Token_EndOfStream; } break;
        case '{':
        {
            EatAllWhiteSpace(Tokenizer);
            Token.Text = Tokenizer->At;

            while(Tokenizer->At[0] && Tokenizer->At[0] != '}')
                ++Tokenizer->At;

            Token.Type = Token_Command;
            Token.Length = Tokenizer->At - Token.Text;
        } break;
        case '}':
        {
            Token.Type = Token_CloseBrace;
        } break;
        case '#':
        {
            Token.Text = Tokenizer->At;
            while(Tokenizer->At[0] && !IsEndOfLine(Tokenizer->At[0]))
                ++Tokenizer->At;

            Token.Type = Token_Comment;
            Token.Length = Tokenizer->At - Token.Text;
        } break;
        default:
        {
            if(IsAlpha(C))
            {
                while(IsAlpha(Tokenizer->At[0]) ||
                      IsNumeric(Tokenizer->At[0]) ||
                      (Tokenizer->At[0] == '+') ||
                      (Tokenizer->At[0] == '-'))
                    ++Tokenizer->At;

                Token.Type = Token_Identifier;
                Token.Length = Tokenizer->At - Token.Text;
            }
            else if(IsNumeric(C))
            {
                if(C == '0' && (Tokenizer->At[0] == 'x' || Tokenizer->At[0] == 'X'))
                {
                    ++Tokenizer->At;
                    while(IsHexadecimal(Tokenizer->At[0]))
                        ++Tokenizer->At;

                    Token.Type = Token_Hex;
                    Token.Length = Tokenizer->At - Token.Text;
                }
                else
                {
                    while(IsNumeric(Tokenizer->At[0]))
                        ++Tokenizer->At;

                    Token.Type = Token_Digit;
                    Token.Length = Tokenizer->At - Token.Text;
                }
            }
            else
            {
                Token.Type = Token_Unknown;
            }
        } break;
    }

    return Token;
}
