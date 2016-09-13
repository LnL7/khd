#ifndef TOKENIZER_H
#define TOKENIZER_H

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

token GetToken(tokenizer *Tokenizer);
bool RequireToken(tokenizer *Tokenizer, token_type DesiredType);
bool TokenEquals(token Token, const char *Match);

#endif
