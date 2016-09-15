#ifndef KHD_TOKENIZE_H
#define KHD_TOKENIZE_H

enum token_type
{
    Token_Identifier,
    Token_Command,

    Token_OpenBrace,
    Token_CloseBrace,

    Token_Hex,
    Token_Digit,
    Token_Comment,

    Token_Unknown,
    Token_EndOfStream,
};

struct token
{
    char *Text;
    unsigned int Length;
    token_type Type;
};

struct tokenizer
{
    char *At;
};

inline bool
IsDot(char C)
{
    bool Result = ((C == '.') ||
                   (C == ','));
    return Result;
}

inline bool
IsEndOfLine(char C)
{
    bool Result = ((C == '\n') ||
                   (C == '\r'));

    return Result;
}

inline bool
IsWhiteSpace(char C)
{
    bool Result = ((C == ' ') ||
                   (C == '\t') ||
                   IsEndOfLine(C));

    return Result;
}

inline bool
IsAlpha(char C)
{
    bool Result = (((C >= 'a') && (C <= 'z')) ||
                   ((C >= 'A') && (C <= 'Z')) ||
                   ((C == '+')) ||
                   ((C == '-')));

    return Result;
}

inline bool
IsNumeric(char C)
{
    bool Result = ((C >= '0') && (C <= '9'));
    return Result;
}

inline bool
IsHexadecimal(char C)
{
    bool Result = (((C >= 'a') && (C <= 'f')) ||
                   ((C >= 'A') && (C <= 'F')) ||
                   (IsNumeric(C)));
    return Result;
}


token GetToken(tokenizer *Tokenizer);
bool RequireToken(tokenizer *Tokenizer, token_type DesiredType);
bool TokenEquals(token Token, const char *Match);

#endif
