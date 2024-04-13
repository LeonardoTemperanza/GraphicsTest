
#include "parser.h"

inline bool IsStartIdent(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

inline bool IsNumeric(char c)
{
    return c >= '0' && c <= '9';
}

inline bool IsMiddleIdent(char c)
{
    return IsStartIdent(c) || IsNumeric(c);
}

int EatAllWhitespace(char** at)
{
    char* ptr = *at;
    int newlines = 0;
    while(**at != '\0')
    {
        if(**at == '\n')
        {
            ++newlines;
            ++*at;
        }
        else if(ptr[0] == ' ' || ptr[0] == '\t')
        {
            ++*at;
        }
        else if(ptr[0] == '/' && ptr[1] == '*')  // Multiline comment
        {
            
        }
    }
    
    return newlines;
}

Token NextToken(Tokenizer* tokenizer)
{
    Tokenizer* t = tokenizer;
    
    int newlines = EatAllWhitespace(&t->at);
    t->numLines += newlines;
    
    if(*t->at == '\0')
    {
        Token endToken = {0};
        endToken.lineNum = t->numLines;
        endToken.kind = TokKind_EOF;
    }
    
    Token token = {0};
    token.lineNum = t->numLines;
    
    if(IsStartIdent(*t->at))
    {
        char* startIdent = t->at;
        ++t->at;
        while(IsMiddleIdent(*t->at)) ++t->at;
        
        String word = {.ptr=startIdent, .len=t->at-startIdent};
        token.word = word;
        
        // Check if it's a keyword
        bool isKeyword = true;
        if(word == "true")
        {
            token.kind = TokKind_True;
        }
        else if(word == "false")
        {
            token.kind = TokKind_False;
        }
        else
            isKeyword = false;
        
        if(!isKeyword)
        {
            token.kind = TokKind_Ident;
        }
    }
    else if(IsNumeric(*t->at))
    {
        TODO;
        // Find out if float value or int value
    }
    else  // Operators and other miscellaneous things
    {
        if(*t->at == ':')
        {
            
        }
        else if(*t->at == ',')
        {
            
        }
        else if(*t->at == '.')
        {
            
        }
    }
    
    return token;
}

void ParseMaterial(Parser* p)
{
    //EatRequiredToken(p, TokKind_Material);
}

void EatRequiredToken(Parser* p, TokenKind token)
{
    
}

void UnexpectedTokenError(Parser* p)
{
    
}

void ParseError(const char* fmt, ...)
{
    
}