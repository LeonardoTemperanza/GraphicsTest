
#pragma once

// This layer will be used for parsing all the textual formats
// in the project (such as materials, binding files, etc.)
// The lexer will be standardized for all formats probably,
// while the parser can vary between these

#include "base.h"

enum TokenKind
{
    TokKind_None = 0,
    TokKind_Error,
    TokKind_Ident,
    TokKind_Colon,
    TokKind_Comma,
    TokKind_Dot,
    TokKind_OpenCurly,
    TokKind_CloseCurly,
    TokKind_FloatConst,
    TokKind_IntConst,
    TokKind_EOF,
    
    // Keywords
    TokKind_True,
    TokKind_False,
    TokKind_MaterialName,
    TokKind_VertexShader,
    TokKind_PixelShader,
    TokKind_Values
};

struct Token
{
    TokenKind kind;
    String text;
    
    int lineNum;
    
    union
    {
        double doubleVal;
        s64    intVal;
    };
};

struct Tokenizer
{
    char* at;
    int numLines;
    
    bool error;
};

struct Parser
{
    Tokenizer t;
    Token token;
};

inline bool IsStartIdent(char c);
inline bool IsNumeric(char c);
inline bool IsMiddleIdent(char c);
inline bool IsWhitespace(char c);

// Returns number of newlines
int EatAllWhitespace(char* at);
Token NextToken(Tokenizer* tokenizer);

struct MatParseResult
{
    
};

struct BindingParseResult
{
    Slice<String> stringIds;
    Slice<String> paths;
};

Parser InitParser(const char* path, Arena* dst);
MatParseResult ParseMaterial();
BindingParseResult ParseBinding();
Token EatRequiredToken(Parser* p, TokenKind token);
void UnexpectedTokenError(Parser* p);
void ParseError(const char* error);
