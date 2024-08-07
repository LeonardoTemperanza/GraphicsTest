
#include "base.cpp"
#include "serialization.h"

#include <iostream>

// DXC includes
#include <windows.h>
#include <Objbase.h>
#include <wrl/client.h>
#include <comdef.h>
#include "dxc_include/dxcapi.h"

// Spirv cross includes
#include "spirvcross_include/spirv_glsl.hpp"

// NOTE: The shader binary works in the following way, there are magic bytes (shader), followed by the version number,
// followed by a header struct that describes the location of the various shaders (binary or not)

const wchar_t* vertexTarget  = L"vs_6_0";
const wchar_t* pixelTarget   = L"ps_6_0";
const wchar_t* computeTarget = L"cs_6_0";

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

const wchar_t* GetHLSLTarget(ShaderKind kind)
{
    const wchar_t* target = L"";
    switch(kind)
    {
        default:                 target = L"";           break;
        case ShaderKind_Vertex:  target = vertexTarget;  break;
        case ShaderKind_Pixel:   target = pixelTarget;   break;
        case ShaderKind_Compute: target = computeTarget; break;
    }
    
    return target;
}

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
void SetWorkingDirRelativeToExe(const char* path);

String CompileDXIL(ShaderKind shaderKind, String hlslSource, String entry, Arena* dst, bool* ok);
String CompileVulkanSpirv(ShaderKind shaderKind, String hlslSource, String entry, Arena* dst, bool* ok);
String CompileGLSL(ShaderKind shaderKind, String vulkanSpirvBinary, Arena* dst, bool* ok);
String CompileOpenglSpirv(ShaderKind shaderKind, String glslSource, Arena* dst, bool* ok);
String CompileMetalIR(ShaderKind shaderKind, String vulkanSpirvBinary, Arena* dst, bool* ok);

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
    
    const char* shaderPath = args[1];
    String ext = GetPathExtension(shaderPath);
    if(ext != "hlsl")
    {
        fprintf(stderr, "File does not have the '.hlsl' extension, so it's assumed not to be a shader.\n");
        return 1;
    }
    
    // Read to the shader source directory
    SetWorkingDirRelativeToExe("../../../Shaders/");
    
    bool ok = true;
    char* nullTerm = LoadEntireFileAndNullTerminate(shaderPath, &ok);
    if(!ok)
    {
        fprintf(stderr, "Error: Could not open file\n");
        return 1;
    }
    
    // Write to the compiled shaders directory
    SetWorkingDirRelativeToExe("../../../Assets/CompiledShaders/");
    
    ScratchArena scratch;
    String shaderSource = {0};
    shaderSource.ptr = nullTerm;
    shaderSource.len = strlen(nullTerm);
    defer { free(nullTerm); };
    
    ParseResult result = {0};
    Slice<ShaderPragma> pragmas = ParseShaderPragmas(nullTerm, scratch);
    int definedStages = 0;
    for(int i = 0; i < pragmas.len; ++i)
    {
        if(pragmas[i].name == "vs")
        {
            auto& stage = result.stages[ShaderKind_Vertex];
            stage.defined = true;
            stage.entry = pragmas[i].param;
            ++definedStages;
        }
        else if(pragmas[i].name == "ps")
        {
            auto& stage = result.stages[ShaderKind_Pixel];
            stage.defined = true;
            stage.entry = pragmas[i].param;
            ++definedStages;
        }
    }
    
    if(definedStages == 0)
    {
        fprintf(stderr, "Error: The shader %s does not have a pipeline stage specifier. If this shader is a vertex shader, add '#pragma vs vertex_main_function_name' at any point of the file. If it's a pixel shader, add '#pragma ps pixel_main_function_name'. The file can also include both shaders.\n", shaderPath);
        return 1;
    }
    
    for(int i = 0; i < ShaderKind_Count; ++i)
    {
        ShaderKind kind = (ShaderKind)i;
        auto& stage = result.stages[i];
        if(stage.defined)
        {
            ScratchArena scratch;
            
            bool ok = true;
            
            String dxil = {0};
            String vulkanSpirv = {0};
            String glslSource = {0};
            
            // D3D12
            if(ok)
                dxil = CompileDXIL(kind, shaderSource, stage.entry, scratch, &ok);
            
            // Vulkan
            if(ok)
                vulkanSpirv = CompileVulkanSpirv(kind, shaderSource, stage.entry, scratch, &ok);
            
            // OpenGL
            if(ok)
                glslSource = CompileGLSL(kind, vulkanSpirv, scratch, &ok);
            
            // Build binary file
            if(ok)
            {
                StringBuilder builder = {0};
                defer { FreeBuffers(&builder); } ;
                
                Append(&builder, "shader");  // Magic bytes
                Put(&builder, (u32)0);       // Version number
                
                ShaderBinaryHeader_v0 header = {0};
                header.shaderKind  = kind;
                header.numMatConstants = 0;
                header.matNames = 0;
                header.matOffsets = 0;
                header.dxil        = sizeof(ShaderBinaryHeader_v0);
                header.vulkanSpirv = header.dxil + dxil.len;
                header.glsl        = header.vulkanSpirv + vulkanSpirv.len;
                
                header.dxilSize        = dxil.len;
                header.vulkanSpirvSize = vulkanSpirv.len;
                header.glslSize        = glslSource.len;
                
                Put(&builder, header);
                Append(&builder, dxil);
                Append(&builder, vulkanSpirv);
                Append(&builder, glslSource);
                
                // Generate output file name
                String pathNoExt = GetPathNoExtension(shaderPath);
                StringBuilder outPath = {0};
                UseArena(&outPath, scratch);
                Append(&outPath, pathNoExt);
                
                // If there is more than one stage in this file,
                // append the stage to the name
                if(definedStages > 1)
                {
                    Append(&outPath, "_");
                    Append(&outPath, GetShaderKindString(kind));
                }
                
                Append(&outPath, ".shader");
                NullTerminate(&outPath);
                
                FILE* outFile = fopen(ToString(&outPath).ptr, "w+b");
                if(!outFile)
                {
                    fprintf(stderr, "Error: Could not write to file\n");
                    return 1;
                }
                
                defer { fclose(outFile); };
                
                WriteToFile(ToString(&builder), outFile);
            }
        }
    }
    
    return 0;
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

void SetWorkingDirRelativeToExe(const char* path)
{
    // TODO: if path doesn't exist, create the directory
    
    StringBuilder assetsPath = {0};
    defer { FreeBuffers(&assetsPath); };
    
    char* exePath = GetExecutablePath();
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
    // Currently in Project/Build/utils
    Append(&assetsPath, path);
    NullTerminate(&assetsPath);
    B_SetCurrentDirectory(ToString(&assetsPath).ptr);
}

// Return the slice of includers as well?
String CompileDXIL(ShaderKind shaderKind, String hlslSource, String entry, Arena* dst, bool* ok)
{
    String binary = {0};
    if(hlslSource.len <= 0) return binary;
    
    // https://simoncoenen.com/blog/programming/graphics/DxcCompiling#compiling
    // https://www.youtube.com/watch?v=tyyKeTsdtmo&t=878s&ab_channel=MicrosoftDirectX12andGraphicsEducation
    
    using namespace Microsoft::WRL;
    
    ComPtr<IDxcUtils> utils;
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.GetAddressOf()));
    
    ComPtr<IDxcIncludeHandler> includeHandler;
    utils->CreateDefaultIncludeHandler(includeHandler.GetAddressOf());
    
    ComPtr<IDxcBlobEncoding> source;
    utils->CreateBlob(hlslSource.ptr, hlslSource.len, CP_UTF8, source.GetAddressOf());
    
    wchar_t* entryWide = ToWCString(entry);
    defer { free(entryWide); };
    
    const wchar_t* args[] =
    {
        L"-E", entryWide,
        L"-T", GetHLSLTarget(shaderKind),
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
    
    // There is some other stuff like extra messages and all that. I imagine those would be the warnings
    ComPtr<IDxcBlobUtf8> errors;
    compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    
    ComPtr<IDxcBlobUtf8> remarks;
    compileResult->GetOutput(DXC_OUT_REMARKS, IID_PPV_ARGS(&remarks), nullptr);
    
    const char* shaderKindStr = GetShaderKindString(shaderKind);
    bool showErrors = errors && errors->GetStringLength();
    bool showRemarks = remarks && remarks->GetStringLength();
    if(showErrors || showRemarks)
        printf("HLSL->DXIL Failed - %s shader compilation messages:\n", shaderKindStr);
    
    if(showErrors)  printf("%s", errors->GetStringPointer());
    if(showRemarks) printf("%s", remarks->GetStringPointer());
    
    if(showErrors || showRemarks)
    {
        *ok = false;
        return binary;
    }
    
    printf("HLSL->DXIL OK - %s shader successfully compiled.\n", shaderKindStr);
    
    ComPtr<IDxcBlob> shaderObj;
    hr = compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderObj), nullptr);
    
    String toCopy = {.ptr=(const char*)shaderObj->GetBufferPointer(), .len=(s64)shaderObj->GetBufferSize()};
    binary = ArenaPushString(dst, toCopy);
    return binary;
}

// Mostly duplicated code, but i suspect the dxil compilation will be more involved in the future
String CompileVulkanSpirv(ShaderKind shaderKind, String hlslSource, String entry, Arena* dst, bool* ok)
{
    String binary = {0};
    if(hlslSource.len <= 0) return binary;
    
    using namespace Microsoft::WRL;
    
    ComPtr<IDxcUtils> utils;
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.GetAddressOf()));
    
    ComPtr<IDxcBlobEncoding> source;
    utils->CreateBlob(hlslSource.ptr, hlslSource.len, CP_UTF8, source.GetAddressOf());
    
    wchar_t* entryWide = ToWCString(entry);
    defer { free(entryWide); };
    
    const wchar_t* args[] =
    {
        L"-spirv",
        L"-Fo", L"model2world_vert.spv",
        L"-E", entryWide,
        L"-T", GetHLSLTarget(shaderKind),
        DXC_ARG_WARNINGS_ARE_ERRORS
    };
    
    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr  = source->GetBufferPointer();
    sourceBuffer.Size = source->GetBufferSize();
    sourceBuffer.Encoding = 0;
    
    ComPtr<IDxcCompiler3> compiler;
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    
    ComPtr<IDxcResult> compileResult;
    HRESULT hr = compiler->Compile(&sourceBuffer, args, ArrayCount(args), nullptr, IID_PPV_ARGS(compileResult.GetAddressOf()));
    
    ComPtr<IDxcBlobUtf8> errors;
    compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    
    ComPtr<IDxcBlobUtf8> remarks;
    compileResult->GetOutput(DXC_OUT_REMARKS, IID_PPV_ARGS(&remarks), nullptr);
    
    const char* shaderKindStr = GetShaderKindString(shaderKind);
    bool showErrors = errors && errors->GetStringLength();
    bool showRemarks = remarks && remarks->GetStringLength();
    if(showErrors || showRemarks)
        printf("HLSL->SPIR-V Failed - %s shader compilation messages:\n", shaderKindStr);
    
    if(showErrors)  printf("%s", errors->GetStringPointer());
    if(showRemarks) printf("%s", remarks->GetStringPointer());
    
    if(showErrors || showRemarks)
    {
        *ok = false;
        return binary;
    }
    
    printf("HLSL->SPIR-V OK - %s shader successfully compiled.\n", shaderKindStr);
    
    ComPtr<IDxcBlob> shaderObj;
    hr = compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderObj), nullptr);
    
    String toCopy = {.ptr=(const char*)shaderObj->GetBufferPointer(), .len=(s64)shaderObj->GetBufferSize()};
    binary = ArenaPushString(dst, toCopy);
    return binary;
}

String CompileGLSL(ShaderKind shaderKind, String vulkanSpirvBinary, Arena* dst, bool* ok)
{
    String binary = {0};
    if(vulkanSpirvBinary.len <= 0) return binary;
    
    using namespace spirv_cross;
    
    // NOTE: Spirv binaries are all supposed to be a multiple of 4
    assert(vulkanSpirvBinary.len % 4 == 0);
    const uint32_t* buf = (uint32_t*)vulkanSpirvBinary.ptr;
    s64 wordCount = vulkanSpirvBinary.len / 4;
    CompilerGLSL compiler(buf, wordCount);
    CompilerGLSL::Options options;
    options.version = 460;
    options.es = false;
    compiler.set_common_options(options);
    
    // Don't really get why I have to call this stuff
    // myself, but ok...
    try
    {
        compiler.build_dummy_sampler_for_combined_images();
    }
    catch(...) {}
    
    try
    {
        compiler.build_combined_image_samplers();
    }
    catch(...) {}
    
    const char* shaderKindStr = GetShaderKindString(shaderKind);
    
    // TODO: Would probably be better to just use the C API
    
    std::string source;
    try
    {
        source = compiler.compile();
        
        // Success
        printf("SPIR-V->GLSL OK - %s shader successfully compiled.\n", shaderKindStr);
    }
    catch(const CompilerError& e)
    {
        *ok = false;
        printf("SPIR-V->GLSL Failed - %s shader compilation messages: %s\n", shaderKindStr, e.what());
    }
    
    binary = ArenaPushString(dst, source);
    return binary;
}

// Use glslangvalidator c++ api
String CompileOpenglSpirv(ShaderKind shaderKind, String glslSource, Arena* dst, bool* ok)
{
    TODO;
    String null = {0};
    return null;
}
