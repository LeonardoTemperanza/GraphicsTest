
#include "os/os_base.cpp"
#include "base.cpp"
#include "asset_system.h"
#include "parser.cpp"

#include <iostream>

struct ShaderMeta
{
    
};

struct ShaderPragma
{
    String name;
    String param;
};

String NextString(char* at);
Slice<ShaderPragma> ParseShaderPragmas(char* source, Arena* dst);
void SetWorkingDirToAssets();

// Usage:
// shader_importer.exe file_to_import.hlsl
// The path is relative to the Assets folder
int main(int argCount, char** args)
{
    constexpr int numArgs = 2;
    if(argCount != numArgs + 1)
    {
        fprintf(stderr, "Incorrect number of arguments.\n");
        return 1;
    }
    
    SetWorkingDirToAssets();
    
    ScratchArena scratch;
    char* shaderSource = LoadEntireFileAndNullTerminate(args[1]);
    defer { free(shaderSource); };
    
    ParseShaderPragmas(shaderSource, scratch);
}

String NextString(char** at)
{
    while(**at == ' ' || **at == '\t') ++*at;
    
    char* start = *at;
    
    while(**at != ' ' && **at != '\t' && **at != '\n' && **at != '\0') ++*at;
    int stringLen = *at - start;
    
    return {.ptr=start, .len=stringLen};
}

Slice<ShaderPragma> ParseShaderPragmas(char* source, Arena* dst)
{
    Array<ShaderPragma> res = {0};
    UseArena(&res, dst);
    
    char* at = source;
    while(*at != '\0')
    {
        if(*at == '#')
        {
            String pragmaKeyword = NextString(&at);
            if(pragmaKeyword == "pragma")
            {
                String pragmaName = NextString(&at);
                if(pragmaName.len > 0)
                {
                    ShaderPragma pragma = {0};
                    pragma.name  = pragmaName;
                    pragma.param = NextString(&at);
                    Append(&res, pragma);
                }
            }
        }
    }
    
    return ToSlice(&res);
}

// @copypasta from main.cpp
void SetWorkingDirToAssets()
{
    StringBuilder assetsPath = {0};
    defer { FreeBuffers(&assetsPath); };
    
    char* exePath = OS_GetExecutablePath();
    defer { free(exePath); };
    
    // Get rid of the .exe file itself in the path
    int len = strlen(exePath);
    int lastSeparator = len - 1;
    for(int i = len-1; i >= 0; --i)
    {
        if(exePath[i] == '/' || exePath[i] == '\\')
        {
            lastSeparator = i;
            break;
        }
    }
    
    String exePathNoFile = {.ptr=exePath, .len=lastSeparator+1};
    Append(&assetsPath, exePathNoFile);
    Append(&assetsPath, "../../Assets/");
    NullTerminate(&assetsPath);
    OS_SetCurrentDirectory(ToString(&assetsPath).ptr);
}