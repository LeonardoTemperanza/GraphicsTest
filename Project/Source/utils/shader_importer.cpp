
#include "os/os_base.cpp"
#include "base.cpp"
#include "asset_system.h"
#include "parser.cpp"

#include <iostream>

#include <windows.h>
#include <Objbase.h>
#include <wrl/client.h>
#include <comdef.h>
#include "dxcapi.h"

#if 0
// Shader binary format spec
// "String" means u32 length followed by a blob of bytes of size 'length'

// v0:
struct Shader
{
    // Metadata
    u8 magicBytes[6] = "shader";
    u32 version = 0;
    ShaderKind kind;
    
    // TODO: Add information about who directly includes this shader, for hot reloading...
    // I guess this would use the raw path.
    
    // Maybe i can provide separate debug versions as well?
    String d3d11Binary;
    String openglSpirvBinary;
};

#endif

struct ParseResult
{
    struct ShaderStage
    {
        bool defined;
        String entry;
    } stages[ShaderKind_Count];
};

struct ShaderPragma
{
    String name;
    String param;
};

String NextString(char* at);
Slice<ShaderPragma> ParseShaderPragmas(char* source, Arena* dst);
void SetWorkingDirToAssets();

// These append the length as u32, followed by the binary
String CompileDXIL(ShaderKind shaderKind, String hlslSource, String entry, Arena* dst, bool* ok);
String CompileVulkanSpirv(ShaderKind shaderKind, String hlslSource, String entry, Arena* dst, bool* ok);
String CompileGLSL(ShaderKind shaderKind, String vulkanSpirvBinary, Arena* dst, bool* ok);
String CompileOpenglSpirv(ShaderKind shaderKind, String glslSource, Arena* dst, bool* ok);

// Usage:
// shader_importer.exe file_to_import.hlsl
// The path is relative to the Assets folder
int main(int argCount, char** args)
{
    constexpr int numArgs = 1;
    if(argCount != numArgs + 1)
    {
        fprintf(stderr, "Incorrect number of arguments.\n");
        return 1;
    }
    
    SetWorkingDirToAssets();
    
    const char* shaderPath = args[1];
    
    ScratchArena scratch;
    char* nullTerm = LoadEntireFileAndNullTerminate(shaderPath);
    if(!nullTerm)
    {
        fprintf(stderr, "Error: Could not open file\n");
        return 1;
    }
    
    String shaderSource = {0};
    shaderSource.ptr = nullTerm;
    shaderSource.len = strlen(nullTerm);
    defer { free(nullTerm); };
    
    ParseResult result = {0};
    Slice<ShaderPragma> pragmas = ParseShaderPragmas(nullTerm, scratch);
    bool anyStageDefined = false;
    for(int i = 0; i < pragmas.len; ++i)
    {
        if(pragmas[i].name == "vs")
        {
            auto& stage = result.stages[ShaderKind_Vertex];
            stage.defined = true;
            stage.entry = pragmas[i].param;
            anyStageDefined = true;
        }
        else if(pragmas[i].name == "ps")
        {
            auto& stage = result.stages[ShaderKind_Pixel];
            stage.defined = true;
            stage.entry = pragmas[i].param;
            anyStageDefined = true;
        }
    }
    
    if(!anyStageDefined)
    {
        fprintf(stderr, "Error: The shader %s does not have a pipeline stage specifier. If this shader is a vertex shader, add '#pragma vs vertex_main_function_name' at any point of the file. If it's a pixel shader, add '#pragma ps pixel_main_function_name'. The file can also include both shaders.\n", shaderPath);
        return 1;
    }
    
    for(int i = 0; i < ShaderKind_Count; ++i)
    {
        auto& stage = result.stages[i];
        if(stage.defined)
        {
            ScratchArena scratch;
            StringBuilder builder = {0};
            
            bool ok = true;
            
            // D3D11
            String dxil = CompileDXIL((ShaderKind)i, shaderSource, stage.entry, scratch, &ok);
            Append(&builder, dxil);
            
            // OpenGL
            
            // ...
            
            // Write to file
            if(ok)
            {
                
            }
        }
    }
}

String NextString(char** at)
{
    EatAllWhitespace(at);
    
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
    while(true)
    {
        EatAllWhitespace(&at);
        
        if(*at == '\0') break;
        
        if(*at == '#')
        {
            ++at;
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
        else
            ++at;
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

String CompileDXIL(ShaderKind shaderKind, String hlslSource, String entry, Arena* dst, bool* ok)
{
    // https://simoncoenen.com/blog/programming/graphics/DxcCompiling#compiling
    // https://www.youtube.com/watch?v=tyyKeTsdtmo&t=878s&ab_channel=MicrosoftDirectX12andGraphicsEducation
    
    using namespace Microsoft::WRL;
    
    String binary = {0};
    
    ComPtr<IDxcUtils> utils;
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.GetAddressOf()));
    
    ComPtr<IDxcIncludeHandler> includeHandler;
    utils->CreateDefaultIncludeHandler(includeHandler.GetAddressOf());
    
    ComPtr<IDxcBlobEncoding> source;
    utils->CreateBlob(hlslSource.ptr, hlslSource.len, CP_UTF8, source.GetAddressOf());
    
    wchar_t* entryWide = ToWCString(entry);
    defer { free(entryWide); };
    
    const wchar_t* target = L"";
    switch(shaderKind)
    {
        default:                target = L"";       break;
        case ShaderKind_Vertex: target = L"vs_6_0"; break;
        case ShaderKind_Pixel:  target = L"ps_6_0"; break;
    }
    
    const char* shaderKindStr = "";
    switch(shaderKind)
    {
        default:                shaderKindStr = "unknown"; break;
        case ShaderKind_Vertex: shaderKindStr = "vertex";  break;
        case ShaderKind_Pixel:  shaderKindStr = "pixel";   break;
    }
    
    const wchar_t* args[] =
    {
        L"-E", entryWide,
        L"-T", target,
        DXC_ARG_WARNINGS_ARE_ERRORS
    };
    
    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr  = source->GetBufferPointer();
    sourceBuffer.Size = source->GetBufferSize();
    sourceBuffer.Encoding = 0;
    
    ComPtr<IDxcCompiler3> compiler;
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    
    fflush(stdout);
    ComPtr<IDxcResult> compileResult;
    HRESULT hr = compiler->Compile(&sourceBuffer, args, ArrayCount(args), nullptr, IID_PPV_ARGS(compileResult.GetAddressOf()));
    
    ComPtr<IDxcBlobUtf8> errors;
    compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    
    if(errors && errors->GetStringLength())
    {
        printf("HLSL->DXIL Compilation Error(s) in %s shader:\n%s", shaderKindStr, errors->GetStringPointer());
        *ok = false;
        return binary;
    }
    
    printf("OK - %s shader successfully compiled to DXIL.\n", shaderKindStr);
    
    ComPtr<IDxcBlob> shaderObj;
    hr = compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderObj), nullptr);
    
    String toCopy = {.ptr=(const char*)shaderObj->GetBufferPointer(), .len=(s64)shaderObj->GetBufferSize()};
    binary = ArenaPushString(dst, toCopy);
    return binary;
}

String CompileOpenglSpirv(ShaderKind shaderKind, String shaderSource, String entry, Arena* dst, bool* ok)
{
    TODO;
    String tmp = {0};
    return tmp;
}
