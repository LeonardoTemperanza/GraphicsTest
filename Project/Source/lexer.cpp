
#include "base.h"
#include "lexer.h"

// This file provides a lexer for a C-like language which can be reusable
// for all sorts of things. It also provides some common functions to easily parse

// Lexing

Slice<Token> LexFile(const char* path, Arena* dst)
{
    bool success = true;
    char* content = LoadEntireFileAndNullTerminate(path, dst, &success);
    if(!success) fprintf(stderr, "Failed to open file '%s'\n", path);
    
    Tokenizer tokenizer = {0};
    tokenizer.at = content;
    tokenizer.start = content;
    
    Array<Token> res = {0};
    UseArena(&res, dst);
    Token token;
    do
    {
        token = GetNextToken(&tokenizer);
        Append(&res, token);
    }
    while(token.kind != Tok_EndOfStream);
    
    return ToSlice(&res);
}

Token GetNextToken(Tokenizer* t)
{
    EatAllWhitespace(t);
    
    Token token = {0};
    token.kind = Tok_EndOfStream;
    token.lineNum = t->lineNum;
    token.startPos = t->at - t->lineStart;
    token.text = {.ptr=t->at, .len=1};
    
    // Multi-character operators
    if(t->at[0] == '-' && t->at[1] == '>')
    {
        token.kind = Tok_Arrow;
        token.text.len = 2;
        t->at += 2;
    }
    else switch(*t->at)
    {
        // Single character operators
        case '\0': token.kind = Tok_EndOfStream;  ++t->at; break;
        case '(':  token.kind = Tok_OpenParen;    ++t->at; break;
        case ')':  token.kind = Tok_CloseParen;   ++t->at; break;
        case '[':  token.kind = Tok_OpenBracket;  ++t->at; break;
        case ']':  token.kind = Tok_CloseBracket; ++t->at; break;
        case '{':  token.kind = Tok_OpenBrace;    ++t->at; break;
        case '}':  token.kind = Tok_CloseBrace;   ++t->at; break;
        case ':':  token.kind = Tok_Colon;        ++t->at; break;
        case ';':  token.kind = Tok_Semicolon;    ++t->at; break;
        case '>':  token.kind = Tok_GT;           ++t->at; break;
        case '<':  token.kind = Tok_LT;           ++t->at; break;
        case '*':  token.kind = Tok_Asterisk;     ++t->at; break;
        case '/':  token.kind = Tok_Slash;        ++t->at; break;
        case ',':  token.kind = Tok_Comma;        ++t->at; break;
        case '.':  token.kind = Tok_Dot;          ++t->at; break;
        
        // String literals
        case '"':
        {
            token.kind = Tok_String;
            
            ++t->at;
            token.text.ptr = t->at;
            while(t->at[0] != '\0' && t->at[0] != '"')
            {
                if(t->at[0] == '\\' && t->at[1] != '\0')
                    ++t->at;
                
                ++t->at;
            }
            
            token.text.len = t->at - token.text.ptr;
            
            if(t->at[0] == '"')
                ++t->at;
            break;
        }
        
        default:
        {
            // Identifiers (keywords are not specified at this stage
            if(IsAlpha(t->at[0]))
            {
                token.kind = Tok_Ident;
                
                while(IsAlpha(t->at[0]) || IsNumber(t->at[0]) || t->at[0] == '_')
                    ++t->at;
                
                token.text.len = t->at - token.text.ptr;
            }
            else if(IsNumber(t->at[0]))
            {
                bool isFloat = false;
                char* decimalPos = t->at;
                while(IsNumber(*decimalPos)) ++decimalPos;
                if(*decimalPos == '.') isFloat = true;
                
                if(isFloat)
                {
                    token.kind = Tok_FloatNum;
                    
                    char* end = t->at;
                    float num = strtof(t->at, &end);
                    
                    token.floatVal = num;
                    token.text.len = end - t->at;
                    t->at = end;
                }
                else
                {
                    token.kind = Tok_IntNum;
                    
                    char* end = t->at;
                    
                    int num = strtol(t->at, &end, 10);
                    token.intVal = num;
                    token.text.len = end - t->at;
                    t->at = end;
                }
            }
            else
            {
                token.kind = Tok_Unknown;
                ++t->at;
            }
            
            break;
        }
    }
    
    return token;
}

void EatAllWhitespace(Tokenizer* t)
{
    while(true)
    {
        if(*t->at == '\n')
        {
            ++t->lineNum;
            t->lineStart = t->at;
            ++t->at;
        }
        else if(t->at[0] == '/' && t->at[1] == '/')  // Single line comment
        {
            while(*t->at != '\n' && *t->at != 0) ++t->at;
            
            if(*t->at == '\n')
            {
                ++t->lineNum;
                t->lineStart = t->at;
                ++t->at;
            }
        }
        else if(t->at[0] == '/' && t->at[1] == '*')  // Multiline comment
        {
            while((t->at[0] != '*' || t->at[1] != '/') && *t->at != 0)
            {
                if(*t->at == '\n')
                {
                    ++t->lineNum;
                    t->lineStart = t->at;
                }
                
                ++t->at;
            }
        }
        else if(IsWhitespace(*t->at))
            ++t->at;
        else
            break;
    }
}

bool IsWhitespace(char c)
{
    return c == '\t' || c == ' ' || c == '\n' || c == '\r';
}

bool IsAlpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool IsNumber(char c)
{
    return c >= '0' && c <= '9';
}

// Parsing

Token* GetNullToken()
{
    static Token null = {.kind=Tok_EndOfStream, .text=StrLit(""), .lineNum=0, .startPos=0};
    return &null;
}

void ParseError(Parser* p, Token* token, const char* message)
{
    if(!p->foundError)
    {
        fprintf(stderr, "%.*s(%d): meta-error: %s\n", (int)p->path.len, p->path.ptr, token->lineNum, message);
        p->at = GetNullToken();
        p->foundError = true;
    }
}

void EatRequiredToken(Parser* p, TokenKind kind)
{
    if(p->at->kind == kind)
    {
        ++p->at;
    }
    else
    {
        ParseError(p, p->at, "Unexpected token");
    }
}
