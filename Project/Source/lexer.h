
#pragma once

// This file provides a lexer for a C-like language which can be reusable
// for all sorts of things. It also provides some common functions to easily parse

// Lexing

enum TokenKind
{
    Tok_Undefined = 0,
    Tok_OpenParen,
    Tok_CloseParen,
    Tok_OpenBracket,
    Tok_CloseBracket,
    Tok_OpenBrace,
    Tok_CloseBrace,
    Tok_Colon,
    Tok_Semicolon,
    Tok_Comma,
    Tok_Dot,
    Tok_Arrow,
    Tok_GT,
    Tok_LT,
    Tok_Asterisk,
    Tok_Slash,
    Tok_String,
    Tok_Ident,
    Tok_IntNum,
    Tok_FloatNum,
    Tok_Unknown,
    
    Tok_EndOfStream
};

struct Token
{
    TokenKind kind;
    String text;
    
    int lineNum;
    int startPos;  // Relative to the start of the line
    
    union
    {
        int intVal;
        float floatVal;
    };
};

struct Tokenizer
{
    char* start;
    char* at;
    int lineNum;
    char* lineStart;
};

Slice<Token> LexFile(const char* path, Arena* arena);
Token GetNextToken(Tokenizer* t);
void EatAllWhitespace(Tokenizer* t);
bool IsWhitespace(char c);
bool IsAlpha(char c);
bool IsNumber(char c);

// Parsing

struct Parser
{
    String path;
    
    Token* at;
    bool foundError;
};

Token* GetNullToken();
void ParseError(Parser* p, Token* token, const char* message);
void EatRequiredToken(Parser* p, TokenKind kind);
