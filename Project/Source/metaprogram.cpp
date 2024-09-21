
#include "base.cpp"
#include "lexer.cpp"
#include "metaprogram.h"

static const char* const headers[] =
{
    "asset_system.h",
    "base.h",
    "core.h",
    "editor.h",
    "input.h",
    "gameplay.h",
};

static const char* const impls[] =
{
    "asset_system.cpp",
    "base.cpp",
    "core.cpp",
    "editor.cpp",
    "input.cpp",
    "main.cpp",
    "unity_build.cpp",
    "gameplay.cpp",
};

static const char* const paths[] =
{
    "asset_system.h",
    "base.h",
    "core.h",
    "editor.h",
    "input.h",
    "asset_system.cpp",
    "base.cpp",
    "core.cpp",
    "editor.cpp",
    "input.cpp",
    "main.cpp",
    "unity_build.cpp",
    "gameplay.h",
    "gameplay.cpp",
};

Array<String> introspectables = {0};

// Current file to write to.
FILE* outputFile;
FILE* introspectionH;
FILE* introspectionImpl;
FILE* serializationH;
FILE* serializationImpl;
void SetOutput(FILE* file)
{
    outputFile = file;
}

// NOTE: These are macros because this way we get special printf compiler errors
#define Println(fmt, ...) fprintf(outputFile, fmt "\n", __VA_ARGS__);
#define Print(fmt, ...) fprintf(outputFile, fmt, __VA_ARGS__);

int main()
{
    InitScratchArenas();
    
    Arena permArena = ArenaVirtualMemInit(GB(2), MB(2));
    
    bool ok = SetCurrentDirectoryRelativeToExe("../Source/");
    assert(ok);
    
    // Load files
    introspectionH    = fopen("generated/introspection.h", "w+");
    introspectionImpl = fopen("generated/introspection.cpp", "w+");
    serializationH    = fopen("generated/serialization.h", "w+");
    serializationImpl = fopen("generated/serialization.cpp", "w+");
    assert(introspectionH && introspectionImpl && serializationH && serializationImpl);
    
    SetOutput(introspectionImpl);
    PrintPreambleComment();
    Println("#include \"generated/introspection.h\"");
    Println("#include \"base.h\"");
    Println("// Including all project headers to pick up struct declarations");
    for(int i = 0; i < ArrayCount(headers); ++i)
        Println("#include \"%s\"", headers[i]);
    Println("");
    
    SetOutput(serializationImpl);
    PrintPreambleComment();
    Println("#include \"generated/serialization.h\"");
    Println("#include \"base.h\"");
    Println("// Including all project headers to pick up struct declarations");
    for(int i = 0; i < ArrayCount(headers); ++i)
        Println("#include \"%s\"", headers[i]);
    Println("");
    
    SetOutput(introspectionH);
    PrintPreambleComment();
    Println("#pragma once");
    Println("#include \"base.h\"");
    Println("");
    
    SetOutput(serializationH);
    PrintPreambleComment();
    Println("#pragma once");
    Println("#include \"base.h\"");
    Println("");
    
    // First pass: gather all introspectables
    auto ptr = ArenaZAllocArray(Slice<Token>, ArrayCount(paths), &permArena);
    Slice<Slice<Token>> tokensPerFile = {.ptr = ptr, .len = 2};
    for(int i = 0; i < ArrayCount(paths); ++i)
    {
        tokensPerFile[i] = LexFile(paths[i], &permArena);
        
        Parser parser = {};
        parser.path = ToLenStr(paths[i]);
        parser.at = &tokensPerFile[i][0];
        Parser* p = &parser;
        
        while(p->at->kind != Tok_EndOfStream)
        {
            while(p->at->kind != Tok_EndOfStream && (p->at->kind != Tok_Ident || p->at->text != "introspect"))
                ++p->at;
            
            if(p->at->kind == Tok_EndOfStream) break;
            
            ++p->at;  // Eat introspect keyword
            EatRequiredToken(p, Tok_OpenParen);
            EatRequiredToken(p, Tok_CloseParen);
            
            if(p->at->kind == Tok_Ident && p->at->text == "struct")
            {
                ++p->at;
                
                if(p->at->kind == Tok_Ident)
                {
                    String text = p->at->text;
                    ++p->at;
                    
                    Append(&introspectables, text);
                }
                else
                    ParseError(p, p->at, "Expecting identifier after 'struct'");
            }
            else
                ParseError(p, p->at, "Expecting 'struct' after 'introspect()'");
        }
    }
    
    SetOutput(introspectionH);
    
    Println("enum MetaType");
    Println("{");
    Println("    Meta_Unknown = 0,");
    Println("    Meta_Int,");
    Println("    Meta_Bool,");
    Println("    Meta_Float,");
    Println("    Meta_Vec3,");
    Println("    Meta_Quat,");
    Println("    Meta_String,");
    // Non-primitive types
    for(int i = 0; i < introspectables.len; ++i)
    {
        Println("    Meta_%.*s,", StrPrintf(introspectables[i]));
    }
    Println("};");
    
    Println("");
    
    Println("struct MetaTypeInfo");
    Println("{");
    Println("    MetaType metaType;");
    Println("};");
    
    Println("");
    
    Println("struct MemberDefinition");
    Println("{");
    Println("    MetaTypeInfo typeInfo;");
    Println("    int offset;");
    Println("    int size;");
    Println("    String name;");
    Println("    const char* cName;");
    Println("    String niceName;");
    Println("    const char* cNiceName;");
    Println("    int version;");
    Println("    bool showEditor;");
    Println("};");
    
    Println("");
    
    Println("struct MetaStruct");
    Println("{");
    Println("    Slice<MemberDefinition> members;");
    Println("    String name;");
    Println("    const char* cName;");
    Println("};");
    
    Println("");
    
    // Macro to handle recursive cases
    Println("// NOTE: The function needs to have a MetaStruct as the first argument.");
    Println("// The variadic arguments are just the remaining arguments to feed to the function.");
    Println("#define Meta_RecursiveCases(functionName, ...) \\");
    for(int i = 0; i < introspectables.len; ++i)
    {
        Println("    case Meta_%.*s: functionName(meta%.*s, __VA_ARGS__); break; \\",
                StrPrintf(introspectables[i]), StrPrintf(introspectables[i]));
    }
    Println("");
    
    // Second pass: Parsing and code generation
    SetOutput(introspectionImpl);
    for(int i = 0; i < ArrayCount(paths); ++i)
    {
        Slice<Token> tokens = LexFile(paths[i], &permArena);
        ParseFile(paths[i], tokens, &permArena);
    }
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
    // Parse parameters to the introspect keyword
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
            
            Println("MemberDefinition _membersOf%.*s[] =", (int)structName->text.len, structName->text.ptr);
            Println("{");
            
            if(p->at->kind == Tok_Ident)
            {
                ++p->at;
                
                EatRequiredToken(p, Tok_OpenBrace);
                ParseMemberList(p, structName->text);
                EatRequiredToken(p, Tok_CloseBrace);
            }
            else
                ParseError(p, p->at, "Expected identifier after 'struct' keyword");
            
            Println("};\n");
            
            Println("MetaStruct meta%.*s =", StrPrintf(structName->text));
            Println("{ {.ptr=_membersOf%.*s, .len=ArrayCount(_membersOf%.*s)}, StrLit(\"%.*s\"), \"%.*s\" };",
                    StrPrintf(structName->text), StrPrintf(structName->text), StrPrintf(structName->text),
                    StrPrintf(structName->text), StrPrintf(structName->text));
            Println("");
        }
        else
            ParseError(p, p->at, "Introspection is only supported on structs at the moment");
    }
    else
        ParseError(p, p->at, "Expected to find 'struct' after 'introspect' keyword");
}

void ParseMemberList(Parser* p, String structName)
{
    ScratchArena scratch;
    
    while(p->at->kind == Tok_Ident)
    {
        bool niceNameDefined = false;
        String niceName = {0};
        bool showEditor = true;
        int memberVersion = 0;
        
        // Optional keywords
        while(true)
        {
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
            else if(p->at->text == "editor_hide")
            {
                ++p->at;
                EatRequiredToken(p, Tok_Semicolon);
                showEditor = false;
            }
            else if(p->at->text == "member_version")
            {
                ++p->at;
                EatRequiredToken(p, Tok_OpenParen);
                
                if(p->at->kind == Tok_IntNum)
                {
                    memberVersion = p->at->intVal;
                    
                    ++p->at;
                }
                else
                    ParseError(p, p->at, "Expecting integer number after member_version(...");
                
                EatRequiredToken(p, Tok_CloseParen);
                EatRequiredToken(p, Tok_Semicolon);
            }
            else if(p->at->text == "const") ++p->at;
            else break;
        }
        
        if(p->at->text == "static")
        {
            // Ignore entire member
            while(p->at->kind != Tok_Semicolon && p->at->kind != Tok_EndOfStream)
                ++p->at;
            if(p->at->kind == Tok_Semicolon)
                ++p->at;
            
            continue;
        }
        
        String typeName = p->at->text;
        ++p->at;
        
        if(typeName == "Slice")
        {
            /*++p->at;
            EatRequiredToken(p, Tok_LT);
            
            if(p->at->kind == Tok_Ident)
            {
                String text = p->at->text;
                ++p->at;
                
                EatRequiredToken(p, Tok_GT);
            }
            else
                ParseError(p, p->at, "Expecting identifier after 'Slice<'");*/
            
            // TODO Ignore for now
            p->at += 5;
            continue;
        }
        if(typeName == "Array")
        {
            // TODO Ignore for now
            p->at += 5;
            continue;
        }
        
        const char* metaType = "Meta_Unknown";
        if     (typeName == "int")    metaType = "Meta_Int";
        else if(typeName == "bool")   metaType = "Meta_Bool";
        else if(typeName == "float")  metaType = "Meta_Float";
        else if(typeName == "Vec3")   metaType = "Meta_Vec3";
        else if(typeName == "Quat")   metaType = "Meta_Quat";
        else if(typeName == "String") metaType = "Meta_String";
        
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
                niceName = GetNiceNameFromMemberName(memberName, scratch);
            
            EatRequiredToken(p, Tok_Semicolon);
            PrintMemberDefinition(structName, metaType, numPtrs, false, false, memberName, niceName, memberVersion, showEditor);
        }
        else
        {
            ParseError(p, p->at, "Could not find member name");
        }
    }
}

const char* BoolString(bool b)
{
    return b ? "true" : "false";
}

void PrintMemberDefinition(String structName, const char* metaType, int numPointers, bool isSlice, bool isString, String name, String niceName, int memberVersion, bool showEditor)
{
    Print("{ ");
    Print("{ %s }, ", metaType);
    Print("offsetof(%.*s, %.*s), ", StrPrintf(structName), StrPrintf(name));
    Print("sizeof(((%.*s*)0)->%.*s), ", StrPrintf(structName), StrPrintf(name));
    Print("StrLit(\"%.*s\"), ", StrPrintf(structName));
    Print("\"%.*s\", ", StrPrintf(structName));
    Print("StrLit(\"%.*s\"), ", StrPrintf(niceName));
    Print("\"%.*s\", ", StrPrintf(niceName));
    Print("%d, ", memberVersion);
    Print("%s", BoolString(showEditor));
    Println("},");
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

void PrintPreambleComment()
{
    Println("");
    Println("/*");
    Println(" * This file was generated by 'metaprogram.cpp'. It contains information about");
    Println(" * various datastructures to be able to use introspection");
    Println("*/");
    Println("");
}
