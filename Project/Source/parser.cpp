
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

inline bool IsWhitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n';
}

int EatAllWhitespace(char** at)
{
    int newlines = 0;
    int commentNestLevel = 0;
    bool inSingleLineComment = false;
    while(true)
    {
        if(**at == '\n')
        {
            inSingleLineComment = false;
            ++newlines;
            ++*at;
        }
        else if((*at)[0] == '/' && (*at)[1] == '/')
        {
            if(commentNestLevel == 0) inSingleLineComment = true;
            *at += 2;
        }
        else if((*at)[0] == '/' && (*at)[1] == '*')  // Multiline comment start
        {
            *at += 2;
            ++commentNestLevel;
        }
        else if((*at)[0] == '*' && (*at)[1] == '/')  // Multiline comment end
        {
            *at += 2;
            --commentNestLevel;
        }
        else if(IsWhitespace(**at) || commentNestLevel > 0 || inSingleLineComment)
        {
            ++*at;
        }
        else
            break;
    }
    
    return newlines;
}

Token NextToken(Tokenizer* tokenizer)
{
    Tokenizer* t = tokenizer;
    
    if(t->error)
    {
        Token errorToken = {0};
        errorToken.lineNum = 0;
        errorToken.kind = TokKind_Error;
        return errorToken;
    }
    
    int newlines = EatAllWhitespace(&t->at);
    t->numLines += newlines;
    
    if(!t->at || *t->at == '\0')
    {
        Token endToken = {0};
        endToken.lineNum = t->numLines;
        endToken.kind = TokKind_EOF;
        return endToken;
    }
    
    Token token = {0};
    token.lineNum = t->numLines;
    
    if(IsStartIdent(*t->at))
    {
        char* startIdent = t->at;
        ++t->at;
        while(IsMiddleIdent(*t->at)) ++t->at;
        
        String text = {.ptr=startIdent, .len=t->at-startIdent};
        token.text = text;
        
        // Check if it's a keyword
        bool isKeyword = true;
        if(text == "true")
        {
            token.kind = TokKind_True;
        }
        else if(text == "false")
        {
            token.kind = TokKind_False;
        }
        else if(text == "material_name")
        {
            token.kind = TokKind_MaterialName;
        }
        else if(text == "vertex_shader")
        {
            token.kind = TokKind_VertexShader;
        }
        else if(text == "pixel_shader")
        {
            token.kind = TokKind_PixelShader;
        }
        else if(text == "values")
        {
            token.kind = TokKind_Values;
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
        char* start = t->at;
        int textLen = 0;
        
        if(*t->at == ':')
        {
            token.kind = TokKind_Colon;
            textLen = 1;
        }
        else if(*t->at == ',')
        {
            token.kind = TokKind_Comma;
            textLen = 1;
        }
        else if(*t->at == '.')
        {
            token.kind = TokKind_Dot;
            textLen = 1;
        }
        else
        {
            token.kind = TokKind_Error;
            textLen = 1;
        }
        
        token.text = {.ptr=t->at, .len=textLen};
        t->at += token.text.len;
    }
    
    return token;
}

Parser InitParser(const char* path, Arena* dst)
{
    Parser parser = {0};
    Tokenizer tokenizer = {0};
    
    String contents = LoadEntireFile(path, dst);
    if(!contents.ptr)
    {
        DebugMessage("Could not open file...\n");
        return parser;
    }
    
    tokenizer.at = (char*)contents.ptr;
    parser.t = tokenizer;
    return parser;
}

void ParseMaterial(Parser* p, Arena* dst)
{
    Token token = NextToken(&p->t);
    switch(token.kind)
    {
        default: ParseError("Unexpected token here\n"); break;
        case TokKind_MaterialName:
        {
            EatRequiredToken(p, TokKind_Colon);
            
            Token ident = NextToken(&p->t);
            if(ident.kind == TokKind_Ident)
            {
                
            }
            else
            {
                
            }
            
            break;
        }
        case TokKind_VertexShader:
        {
            EatRequiredToken(p, TokKind_Colon);
            
            break;
        }
        case TokKind_PixelShader:
        {
            EatRequiredToken(p, TokKind_Colon);
            
            break;
        }
        case TokKind_Values:
        {
            EatRequiredToken(p, TokKind_OpenCurly);
            
            
            
            break;
        }
    }
}

BindingParseResult ParseBinding(Parser* p, Arena* dst)
{
    BindingParseResult result = {0};
    Array<String> ids;
    defer { Free(&ids); };
    Array<String> paths;
    defer { Free(&paths); };
    
    Token token = NextToken(&p->t);
    while(token.kind != TokKind_EOF && token.kind != TokKind_Error)
    {
        Token stringId = EatRequiredToken(p, TokKind_Ident);
        EatRequiredToken(p, TokKind_Colon);
        Token path = EatRequiredToken(p, TokKind_Ident);
        
        Append(&ids, ArenaPushString(dst, stringId.text));
        Append(&paths, ArenaPushString(dst, path.text));
    }
    
    if(p->t.error) return result;
    
    result.stringIds = CopyToArena(&ids, dst);
    result.paths     = CopyToArena(&paths, dst);
    return result;
}

Token EatRequiredToken(Parser* p, TokenKind token)
{
    Token next = NextToken(&p->t);
    if(next.kind != token)
    {
        Token errorToken = {0};
        errorToken.kind = TokKind_Error;
        p->t.error = true;
    }
    else
    {
        
    }
    
    return next;
}

void ParseError(const char* error)
{
    // TODO better error handling!
    DebugMessage(error);
}
