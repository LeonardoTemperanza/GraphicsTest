
#include "parser.h"

inline bool IsStartIdent(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

inline bool IsMiddleIdent(char c)
{
    return IsStartIdent(c) || (c >= '0' && c <= '9');
}

int EatAllWhitespace(char** at)
{
    int newlines = 0;
    while(**at != '\0')
    {
        if(**at == '\n') ++newlines;
        
    }
    
    return newlines;
}

Token NextToken(Tokenizer* tokenizer)
{
    Tokenizer* t = tokenizer;
    
    int newlines = EatAllWhitespace(&t->at);
    t->numLines += newlines;
    
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
            
        }
        else if(word == "false")
        {
            
        }
        else
            isKeyword = false;
        
        if(!isKeyword)
        {
            token.kind = TokKind_Ident;
        }
    }
    else if(true)
    {
        
    }
    
    return {0};
}

void ParseMaterial(Parser* p)
{
    //EatRequiredToken(p, TokKind_Material);
}

void EatRequiredToken(Parser* p, TokenKind token);
void UnexpectedTokenError(Parser* p);
void ParseError(const char* fmt, ...);