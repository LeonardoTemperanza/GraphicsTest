
#include "base.cpp"

static const char* const paths[] =
{
    "asset_system.h",
    "asset_system.cpp",
    "base.h",
    "base.cpp",
    "core.h",
    "core.cpp",
    "editor.h",
    "editor.cpp",
    "input.h",
    "input.cpp",
    "main.cpp",
    "unity_build.cpp"
};

enum TokenKind
{
    Tok_OpenParen,
    Tok_CloseParen,
    Tok_OpenBracket,
    Tok_CloseBracket,
    Tok_OpenBrace,
    Tok_CloseBrace,
    Tok_Colon,
    Tok_Semicolon,
    Tok_Asterisk,
    Tok_String,
    Tok_Ident,
    Tok_Unknown,
    
    Tok_EndOfStream
};

struct Token
{
    TokenKind kind;
    String text;
    
    int lineNum;
    
    // Relative to the start of the line
    int startPos;
    
    static const Token null;
};

struct Tokenizer
{
    const char* path;
    
    char* start;
    char* at;
    int lineNum;
    char* lineStart;
};

struct Parser
{
    String path;
    
    Token* at;
    bool foundError;
};

Slice<Token> LexFile(const char* path, Arena* arena);
Token GetNextToken(Tokenizer* t);
void EatAllWhitespace(Tokenizer* t);
bool IsWhitespace(char c);
bool IsAlpha(char c);
bool IsNumber(char c);
void ParseFile(const char* path, Slice<Token> tokens, Arena* dst);
void ParseIntrospection(Parser* p);
void ParseMemberList(Parser* p, String structName);
Token* GetNullToken();
void ParseError(Parser* p, Token* token, const char* message);
void EatRequiredToken(Parser* p, TokenKind kind);

void PrintMemberDefinition(String structName, const char* metaType, int numPointers, bool isSlice, bool isString,
                           String name, String niceName, bool showEditor);
// Converts camel case to regular sentence, like this:
// From: "thisIsAnExampleTest"
// To:   "This Is An Example Test"
String GetNiceNameFromMemberName(String memberName, Arena* dst);

Array<String> introspectables = {0};

int main()
{
    Arena permArena = ArenaVirtualMemInit(GB(2), MB(2));
    
    bool ok = SetCurrentDirectoryRelativeToExe("../Source/");
    assert(ok);
    
    printf("\n");
    
    printf("#pragma once\n\n");
    
    printf("/*\n");
    printf(" * This file was generated by 'metaprogram.cpp'. It contains information about\n");
    printf(" * various datastructures to be able to use introspection\n");
    printf("*/\n");
    
    printf("\n");
    
    printf("#include \"base.h\"");
    
    printf("\n");
    printf("\n");
    
    printf("enum MetaType\n");
    printf("{\n");
    printf("    Meta_Unknown = 0,\n");
    printf("    Meta_Int,\n");
    printf("    Meta_Bool,\n");
    printf("    Meta_Float,\n");
    printf("    Meta_Vec3,\n");
    printf("    Meta_Quat,\n");
    printf("};\n");
    
    printf("\n");
    
    printf("struct MetaTypeInfo\n");
    printf("{\n");
    printf("    MetaType metaType;\n");
    printf("    int numPointers;\n");
    printf("    bool isSlice;\n");
    printf("    bool isString;\n");
    printf("};\n");
    
    printf("\n");
    
    printf("struct MemberDefinition\n");
    printf("{\n");
    printf("    MetaTypeInfo typeInfo;\n");
    printf("    int offset;\n");
    printf("    String name;\n");
    printf("    const char* cName;\n");
    printf("    String niceName;\n");
    printf("    const char* cNiceName;\n");
    printf("    bool showEditor;\n");
    printf("};\n");
    
    printf("\n");
    
    printf("struct MetaStruct\n");
    printf("{\n");
    printf("    Slice<MemberDefinition> members;\n");
    printf("    String name;\n");
    printf("    const char* cName;\n");
    printf("};\n");
    
    printf("\n");
    
    // TODO: Two passes, first pass to get all introspectables
    // and second pass to do actual parsing
    for(int i = 0; i < ArrayCount(paths); ++i)
    {
        Slice<Token> tokens = LexFile(paths[i], &permArena);
        ParseFile(paths[i], tokens, &permArena);
    }
}

Slice<Token> LexFile(const char* path, Arena* dst)
{
    bool success = true;
    char* content = LoadEntireFileAndNullTerminate(path, dst, &success);
    if(!success) fprintf(stderr, "Failed to open file '%s'\n", path);
    
    Tokenizer tokenizer = {0};
    tokenizer.at = content;
    tokenizer.start = content;
    tokenizer.path = path;
    
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
    
    switch(*t->at)
    {
        case '\0': token.kind = Tok_EndOfStream;  ++t->at; break;
        case '(':  token.kind = Tok_OpenParen;    ++t->at; break;
        case ')':  token.kind = Tok_CloseParen;   ++t->at; break;
        case '[':  token.kind = Tok_OpenBracket;  ++t->at; break;
        case ']':  token.kind = Tok_CloseBracket; ++t->at; break;
        case '{':  token.kind = Tok_OpenBrace;    ++t->at; break;
        case '}':  token.kind = Tok_CloseBrace;   ++t->at; break;
        case ':':  token.kind = Tok_Colon;        ++t->at; break;
        case ';':  token.kind = Tok_Semicolon;    ++t->at; break;
        case '*':  token.kind = Tok_Asterisk;     ++t->at; break;
        
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
            if(IsAlpha(t->at[0]))
            {
                token.kind = Tok_Ident;
                
                while(IsAlpha(t->at[0]) || IsNumber(t->at[0]) || t->at[0] == '_')
                    ++t->at;
                
                token.text.len = t->at - token.text.ptr;
            }
#if 0
            else if(IsNumber(t->at[0]))
            {
                
            }
#endif
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
        else if(*t->at == '/' && *t->at == '/')  // Single line comment
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

void ParseFile(const char* path, Slice<Token> tokens, Arena* dst)
{
    Parser parser = {0};
    parser.at = tokens.ptr;
    parser.path = GetFullPath(path, dst);
    Parser* p = &parser;
    
    bool parsing = true;
    while(parsing)
    {
        switch(p->at->kind)
        {
            case Tok_EndOfStream:
            {
                parsing = false;
                break;
            }
            case Tok_Unknown:
            {
                ++p->at;
                break;
            }
            case Tok_Ident:
            {
                if(p->at->text == "introspect")
                {
                    ++p->at;
                    // Introspectable found
                    ParseIntrospection(p);
                }
                else
                    ++p->at;
                
                break;
            }
            default:
            {
                ++p->at;
                break;
            }
        }
    }
}

void ParseIntrospection(Parser* p)
{
    if(p->at->kind == Tok_OpenParen)
    {
        while(true)
        {
            
            ++p->at;
            
            if(p->at->kind == Tok_CloseParen)
            {
                ++p->at;
                break;
            }
            else if(p->at->kind != Tok_EndOfStream)
                break;
        }
    }
    
    if(p->at->kind == Tok_Ident)
    {
        if(p->at->text == "struct")
        {
            ++p->at;
            
            Token* structName = p->at;
            printf("MemberDefinition _membersOf%.*s[] =\n", (int)structName->text.len, structName->text.ptr);
            printf("{\n");
            
            if(p->at->kind == Tok_Ident)
            {
                ++p->at;
                
                Append(&introspectables, structName->text);
                EatRequiredToken(p, Tok_OpenBrace);
                ParseMemberList(p, structName->text);
                EatRequiredToken(p, Tok_CloseBrace);
            }
            else
                ParseError(p, p->at, "Expected identifier after 'struct' keyword");
            
            printf("};\n\n");
            
            printf("MetaStruct meta%.*s =\n", StrPrintf(structName->text));
            printf("{ {.ptr=_membersOf%.*s, .len=ArrayCount(_membersOf%.*s)}, StrLit(\"%.*s\"), \"%.*s\" };\n",
                   StrPrintf(structName->text), StrPrintf(structName->text), StrPrintf(structName->text),
                   StrPrintf(structName->text), StrPrintf(structName->text));
            printf("\n");
        }
        else
            ParseError(p, p->at, "Introspection is only supported on structs at the moment");
    }
    else
        ParseError(p, p->at, "Expected to find 'struct' after 'introspect' keyword");
}

void ParseMemberList(Parser* p, String structName)
{
    while(p->at->kind == Tok_Ident)
    {
        bool niceNameDefined = false;
        String niceName = {0};
        bool showEditor = true;
        
        // Optional keywords
        {
            // nice_name keyword
            if(p->at->text == "nice_name")
            {
                ++p->at;
                EatRequiredToken(p, Tok_OpenParen);
                
                if(p->at->kind == Tok_String)
                {
                    niceName = p->at->text;
                    niceNameDefined = true;
                    ++p->at;
                }
                else
                    ParseError(p, p->at, "Expecting string literal after nice_name keyword");
                
                EatRequiredToken(p, Tok_CloseParen);
                EatRequiredToken(p, Tok_Semicolon);
            }
            
            // editor_hide keyword
            if(p->at->text == "editor_hide")
            {
                ++p->at;
                EatRequiredToken(p, Tok_Semicolon);
                showEditor = false;
            }
        }
        
        // Ignore const keyword
        if(p->at->text == "const")
            ++p->at;
        
        if(p->at->text == "static")
        {
            // Ignore entire member
            while(p->at->kind != Tok_Semicolon && p->at->kind != Tok_EndOfStream)
                ++p->at;
            if(p->at->kind == Tok_Semicolon)
                ++p->at;
            
            continue;
        }
        
        // Ignore const keyword
        if(p->at->text == "const")
            ++p->at;
        
        String typeName = p->at->text;
        ++p->at;
        
        const char* metaType = "Meta_Unknown";
        if(typeName == "int")
        {
            metaType = "Meta_Int";
        }
        else if(typeName == "bool")
        {
            metaType = "Meta_Bool";
        }
        else if(typeName == "float")
        {
            metaType = "Meta_Float";
        }
        else if(typeName == "Vec3")
        {
            metaType = "Meta_Vec3";
        }
        else if(typeName == "Quat")
        {
            metaType = "Meta_Quat";
        }
        
        int numPtrs = 0;
        while(p->at->kind == Tok_Asterisk)
        {
            ++numPtrs;
            ++p->at;
        }
        
        if(p->at->kind == Tok_Ident)
        {
            String memberName = p->at->text;
            ++p->at;
            
            if(!niceNameDefined)
            {
                ScratchArena scratch;
                niceName = GetNiceNameFromMemberName(memberName, scratch);
            }
            
            EatRequiredToken(p, Tok_Semicolon);
            PrintMemberDefinition(structName, metaType, numPtrs, false, false, memberName, niceName, showEditor);
        }
        else
        {
            ParseError(p, p->at, "Could not find member name");
        }
    }
}

Token* GetNullToken()
{
    static Token null = {.kind=Tok_EndOfStream, .text=StrLit(""), .lineNum=0, .startPos=0};
    return &null;
}

void ParseError(Parser* p, Token* token, const char* message)
{
    fprintf(stderr, "%.*s(%d): meta-error: %s\n", (int)p->path.len, p->path.ptr, token->lineNum, message);
    p->at = GetNullToken();
    p->foundError = true;
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

const char* BoolString(bool b)
{
    return b ? "true" : "false";
}

void PrintMemberDefinition(String structName, const char* metaType, int numPointers, bool isSlice, bool isString, String name, String niceName, bool showEditor)
{
    printf("{ { %s, %d, %s, %s }, offsetof(%.*s, %.*s), StrLit(\"%.*s\"), \"%.*s\", StrLit(\"%.*s\"), \"%.*s\", %s },\n",
           metaType, numPointers, BoolString(isSlice), BoolString(isString), StrPrintf(structName), StrPrintf(name),
           StrPrintf(name), StrPrintf(name), StrPrintf(niceName), StrPrintf(niceName), BoolString(showEditor));
}

// TODO: put this is the base layer
char ToUpperCase(char c)
{
    if(c >= 'a' && c <= 'z')
        c += 'A' - 'a';
    
    return c;
}

String GetNiceNameFromMemberName(String memberName, Arena* dst)
{
    if(memberName.len <= 0) return {0};
    
    StringBuilder builder = {0};
    UseArena(&builder, dst);
    
    Append(&builder, ToUpperCase(memberName[0]));
    
    int lastIdx = 1;
    for(int i = 1; i < memberName.len; ++i)
    {
        if(memberName[i] >= 'A' && memberName[i] <= 'Z')
        {
            String toAppend = {.ptr=memberName.ptr + lastIdx, .len=i - lastIdx};
            Append(&builder, toAppend);
            Append(&builder, ' ');
            lastIdx = i;
        }
    }
    
    String toAppend = {.ptr=memberName.ptr + lastIdx, .len=memberName.len - lastIdx};
    Append(&builder, toAppend);
    
    return ToString(&builder);
}
