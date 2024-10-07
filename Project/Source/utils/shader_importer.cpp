
#include "base.cpp"
#include "serialization.h"

#include <iostream>

// Direct3D
#include <windows.h>

// DX12 includes
#include <Objbase.h>
#include <wrl/client.h>
#include <comdef.h>
#include "dxc_include/dxcapi.h"
#include "dxc_include/D3d12shader.h"

// DX11 includes
#include <d3d11.h>
#include <d3dcompiler.h>

using namespace Microsoft::WRL;

// Spirv cross includes
#include "spirvcross_include/spirv_glsl.hpp"

// NOTE: The shader binary works in the following way, there are magic bytes ("shader"), followed by the version number,
// followed by a header struct that describes the location of the various shaders (binary or not)

// HLSL shader models to use when compiling for d3d11
const char* d3d11_vertexTarget  = "vs_5_0";
const char* d3d11_pixelTarget   = "ps_5_0";
const char* d3d11_computeTarget = "cs_5_0";

// HLSL shader models to use when compiling to DXIL (d3d12)
const wchar_t* d3d12_vertexTarget  = L"vs_6_0";
const wchar_t* d3d12_pixelTarget   = L"ps_6_0";
const wchar_t* d3d12_computeTarget = L"cs_6_0";

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

inline bool IsWhitespace(char c);
int EatAllWhitespace(char** at);
const wchar_t* D3D12_GetHLSLTarget(ShaderKind kind);
const char* D3D11_GetHLSLTarget(ShaderKind kind);

enum DxcCompilationKind
{
    ToDxil,
    ToSpirv
};

String CompileHLSL(ShaderKind shaderKind, String hlslSource, String entry, Arena* dst, bool* ok, DxcCompilationKind compileTo,
                   ComPtr<ID3D12ShaderReflection>& outReflection);
String CompileHLSLForD3D11(ShaderKind shaderKind, String hlslSource, String entry, Arena* dst, bool* ok);
String CompileToGLSL(ShaderKind shaderKind, String vulkanSpirvBinary, Arena* dst, bool* ok);
void BuildBinary(String d3d11Bytecode, String dxil, String vulkanSpirv, String glsl, ComPtr<ID3D12ShaderReflection>& reflection, const char* shaderPath, ShaderKind kind, int definedStages);

// Usage:
// shader_importer.exe file_to_import.hlsl
// The path is relative to the Assets folder
int main(int argCount, char** args)
{
    InitScratchArenas();
    InitPermArena();
    
    constexpr int numArgs = 1;
    if(argCount != numArgs + 1)
    {
        fprintf(stderr, "Incorrect number of arguments.\n");
        return 1;
    }
    
    const char* shaderPath = args[1];
    String ext = GetPathExtension(shaderPath);
    if(ext != "hlsl" && ext != "hlsli")
    {
        fprintf(stderr, "File does not have the '.hlsl' or '.hlsli' extension, so it's assumed not to be a shader.\n");
        return 1;
    }
    
    // Read to the shader source directory
    SetWorkingDirRelativeToExe("../../../Shaders/");
    
    bool ok = true;
    
    Arena arena = ArenaVirtualMemInit(GB(2), MB(4));
    char* nullTerm = LoadEntireFileAndNullTerminate(shaderPath, &arena, &ok);
    if(!ok)
    {
        fprintf(stderr, "Error: Could not open file\n");
        return 1;
    }
    
    ScratchArena scratch;
    String shaderSource = {0};
    shaderSource.ptr = nullTerm;
    shaderSource.len = strlen(nullTerm);
    
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
        else if(pragmas[i].name == "header")
        {
            // Shader which have #pragma header should not
            // be compiled by themselves
            return 0;
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
            bool ok = true;
            
            String d3d11Bytecode = {0};
            String dxil = {0};
            String vulkanSpirv = {0};
            String glslSource = {0};
            
            ComPtr<ID3D12ShaderReflection> reflection;
            
            // D3D11
            if(ok)
                d3d11Bytecode = CompileHLSLForD3D11(kind, shaderSource, stage.entry, scratch, &ok);
            
            // D3D12
            if(ok)
                dxil = CompileHLSL(kind, shaderSource, stage.entry, scratch, &ok, ToDxil, reflection);
            
            // Vulkan
            if(ok)
                vulkanSpirv = CompileHLSL(kind, shaderSource, stage.entry, scratch, &ok, ToSpirv, reflection);
            
            // OpenGL
            if(ok)
                glslSource = CompileToGLSL(kind, vulkanSpirv, scratch, &ok);
            
            if(ok)
                BuildBinary(d3d11Bytecode, dxil, vulkanSpirv, glslSource, reflection, shaderPath, kind, definedStages);
        }
    }
    
    return 0;
}

String CompileHLSL(ShaderKind shaderKind, String hlslSource, String entry, Arena* dst, bool* ok, DxcCompilationKind compileTo, ComPtr<ID3D12ShaderReflection>& outReflection)
{
    String binary = {0};
    if(hlslSource.len <= 0) return binary;
    
    // https://simoncoenen.com/blog/programming/graphics/DxcCompiling#compiling
    // https://www.youtube.com/watch?v=tyyKeTsdtmo&t=878s&ab_channel=MicrosoftDirectX12andGraphicsEducation
    
    ComPtr<IDxcUtils> utils;
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.GetAddressOf()));
    
    ComPtr<IDxcIncludeHandler> defaultIncludeHandler;
    utils->CreateDefaultIncludeHandler(defaultIncludeHandler.GetAddressOf());
    
    ComPtr<IDxcBlobEncoding> source;
    utils->CreateBlob(hlslSource.ptr, hlslSource.len, CP_UTF8, source.GetAddressOf());
    
    wchar_t* entryWide = ToWCString(entry);
    defer { free(entryWide); };
    
    Array<const wchar_t*> args = {0};
    defer { Free(&args); };
    
    Append(&args, L"-E");
    Append(&args, (const wchar_t*)entryWide);
    Append(&args, L"-T");
    Append(&args, D3D12_GetHLSLTarget(shaderKind));
    if(compileTo == ToSpirv)
        Append(&args, L"-spirv");
    
    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr  = source->GetBufferPointer();
    sourceBuffer.Size = source->GetBufferSize();
    sourceBuffer.Encoding = 0;
    
    ComPtr<IDxcCompiler3> compiler;
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    
    fflush(stdout);
    
    ComPtr<IDxcResult> compileResult;
    HRESULT hr = compiler->Compile(&sourceBuffer, args.ptr, args.len, defaultIncludeHandler.Get(), IID_PPV_ARGS(compileResult.GetAddressOf()));
    
    // There is some other stuff like extra messages and all that. I imagine those would be the warnings
    ComPtr<IDxcBlobUtf8> errors;
    compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    
    ComPtr<IDxcBlobUtf8> remarks;
    compileResult->GetOutput(DXC_OUT_REMARKS, IID_PPV_ARGS(&remarks), nullptr);
    
    const char* shaderKindStr = GetShaderKindString(shaderKind);
    bool showErrors = errors && errors->GetStringLength();
    bool showRemarks = remarks && remarks->GetStringLength();
    if(showErrors || showRemarks)
    {
        if(compileTo == ToSpirv)
            printf("HLSL->SPIRV Failed - ");
        else
            printf("HLSL->DXIL Failed - ");
        printf("%s shader compilation messages:\n", shaderKindStr);
    }
    
    if(showErrors)  printf("%s", errors->GetStringPointer());
    if(showRemarks) printf("%s", remarks->GetStringPointer());
    
    if(showErrors || showRemarks)
    {
        *ok = false;
        return binary;
    }
    
    // Compilation successful
    
    bool isReflectionSupported = compileTo == ToDxil;
    if(isReflectionSupported)
    {
        ComPtr<IDxcBlob> reflectionBlob;
        hr = compileResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionBlob), nullptr);
        assert(SUCCEEDED(hr));
        
        DxcBuffer reflectionBuffer;
        reflectionBuffer.Ptr = reflectionBlob->GetBufferPointer();
        reflectionBuffer.Size = reflectionBlob->GetBufferSize();
        reflectionBuffer.Encoding = 0;
        
        hr = utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(outReflection.GetAddressOf()));
        assert(SUCCEEDED(hr));
    }
    
    if(compileTo == ToSpirv)
        printf("HLSL->SPIRV OK - ");
    else
        printf("HLSL->DXIL OK - ");
    printf("%s shader successfully compiled.\n", shaderKindStr);
    
    ComPtr<IDxcBlob> shaderObj;
    hr = compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderObj), nullptr);
    
    String toCopy = {.ptr=(const char*)shaderObj->GetBufferPointer(), .len=(s64)shaderObj->GetBufferSize()};
    binary = ArenaPushString(dst, toCopy);
    return binary;
}

String CompileHLSLForD3D11(ShaderKind shaderKind, String hlslSource, String entry, Arena* dst, bool* ok)
{
    ScratchArena scratch(dst);
    
    StringBuilder builder = {};
    UseArena(&builder, scratch);
    Append(&builder, entry);
    NullTerminate(&builder);
    String nullTermEntry = ToString(&builder); 
    
    DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
    // TODO: Remove this on release build
    shaderFlags |= D3DCOMPILE_DEBUG;
    
    ID3DBlob* errorBlob = nullptr;
    ID3DBlob* bytecode  = nullptr;
    // TODO: Add source name for better error messages
    HRESULT hr = D3DCompile(hlslSource.ptr,
                            hlslSource.len,
                            nullptr,  // Optional shader name
                            nullptr,  // Optional defines
                            D3D_COMPILE_STANDARD_FILE_INCLUDE,  // Optional include handler
                            nullTermEntry.ptr,
                            D3D11_GetHLSLTarget(shaderKind),
                            shaderFlags, // Compilation flags
                            0,           // Effect compilation flags
                            &bytecode,
                            &errorBlob);
    
    if(FAILED(hr) && errorBlob)
    {
        printf("HLSL->D3D11 Bytecode Failed - ");
        printf("%s shader compilation messages:\n", GetShaderKindString(shaderKind));
        printf("%s", (char*)errorBlob->GetBufferPointer());
        errorBlob->Release();
        *ok = false;
        return {};
    }
    
    assert(SUCCEEDED(hr));
    
    printf("HLSL->D3D11 OK - ");
    printf("%s shader successfully compiled.\n", GetShaderKindString(shaderKind));
    return {.ptr=(const char*)bytecode->GetBufferPointer(), .len=(s64)bytecode->GetBufferSize()};
}

String CompileToGLSL(ShaderKind shaderKind, String vulkanSpirvBinary, Arena* dst, bool* ok)
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
    catch(const CompilerError& e)
    {
        printf("Error while building dummy samplers for combined images: %s\n", e.what());
    }
    
    try
    {
        compiler.build_combined_image_samplers();
        
        int location = 0;
        
        ShaderResources resources = compiler.get_shader_resources();
        int numTextures = resources.separate_images.size();
        int numSamplers = resources.separate_samplers.size();
        
        //printf("texs: %d, samplers: %d\n", numTextures, numSamplers);
        
        for (auto &remap : compiler.get_combined_image_samplers())
        {
            compiler.set_name(remap.combined_id, join("SPIRV_Cross_Combined_", compiler.get_name(remap.image_id),
                                                      compiler.get_name(remap.sampler_id)));
            
            int textureBinding = compiler.get_decoration(remap.image_id, spv::DecorationBinding);
            int samplerBinding = compiler.get_decoration(remap.image_id, spv::DecorationBinding);
            compiler.set_decoration(remap.combined_id, spv::DecorationLocation, textureBinding);
        }
    }
    catch(const CompilerError& e)
    {
        printf("Error while building combined image samplers: %s\n", e.what());
    }
    
    const char* shaderKindStr = GetShaderKindString(shaderKind);
    
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

void BuildBinary(String d3d11Bytecode, String dxil, String vulkanSpirv, String glsl, ComPtr<ID3D12ShaderReflection>& reflection, const char* shaderPath, ShaderKind kind, int definedStages)
{
    ScratchArena scratch;
    
    StringBuilder builder = {0};
    defer { FreeBuffers(&builder); } ;
    
    constexpr u32 version = 1;
    Append(&builder, "shader");   // Magic bytes
    Put(&builder, (u32)version);  // Version number
    
    ShaderBinaryHeader_v1 header = {0};
    header.v0.shaderKind  = kind;
    header.v0.numMatConstants = 0;
    header.v0.matNames = 0;
    header.v0.matOffsets = 0;
    
#if 0
    // Get reflection info
    D3D12_SHADER_DESC shaderDesc;
    reflection->GetDesc(&shaderDesc);
    
    // Iterate over the constant buffer
    for(int i = 0; i < shaderDesc.ConstantBuffers; ++i)
    {
        ID3D12ShaderReflectionConstantBuffer* constBuffer = reflection->GetConstantBufferByIndex(i);
        D3D12_SHADER_BUFFER_DESC bufferDesc;
        constBuffer->GetDesc(&bufferDesc);
        
        printf("Constant Buffer %u: %s, Variables: %u\n", i, bufferDesc.Name, bufferDesc.Variables);
        
        for (UINT j = 0; j < bufferDesc.Variables; j++)
        {
            ID3D12ShaderReflectionVariable* variable = constBuffer->GetVariableByIndex(j);
            D3D12_SHADER_VARIABLE_DESC variableDesc;
            variable->GetDesc(&variableDesc);
            
            printf("  Variable %u: %s, Size: %u, Offset: %u\n", j, variableDesc.Name, variableDesc.Size, variableDesc.StartOffset);
        }
    }
#endif
    
    header.v0.dxil            = sizeof(ShaderBinaryHeader_v1);
    header.v0.dxilSize        = dxil.len;
    header.v0.vulkanSpirv     = header.v0.dxil + dxil.len;
    header.v0.vulkanSpirvSize = vulkanSpirv.len;
    header.v0.glsl            = header.v0.vulkanSpirv + vulkanSpirv.len;
    header.v0.glslSize        = glsl.len;
    header.d3d11Bytecode      = header.v0.glsl + glsl.len;
    header.d3d11BytecodeSize  = d3d11Bytecode.len;
    Put(&builder, header);
    Append(&builder, dxil);
    Append(&builder, vulkanSpirv);
    Append(&builder, glsl);
    Append(&builder, d3d11Bytecode);
    
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
    
    // Write to the compiled shaders directory
    SetWorkingDirRelativeToExe("../../../Assets/CompiledShaders/");
    
    FILE* outFile = fopen(ToString(&outPath).ptr, "w+b");
    if(!outFile)
    {
        fprintf(stderr, "Error: Could not write to file\n");
        exit(1);
    }
    
    defer { fclose(outFile); };
    
    WriteToFile(ToString(&builder), outFile);
    
    // Set it back
    SetWorkingDirRelativeToExe("../../../Shaders/");
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

const wchar_t* D3D12_GetHLSLTarget(ShaderKind kind)
{
    const wchar_t* target = L"";
    switch(kind)
    {
        default:                 target = L"";                 break;
        case ShaderKind_Vertex:  target = d3d12_vertexTarget;  break;
        case ShaderKind_Pixel:   target = d3d12_pixelTarget;   break;
        case ShaderKind_Compute: target = d3d12_computeTarget; break;
    }
    
    return target;
}

const char* D3D11_GetHLSLTarget(ShaderKind kind)
{
    const char* target = "";
    switch(kind)
    {
        default:                 target = "";                  break;
        case ShaderKind_Vertex:  target = d3d11_vertexTarget;  break;
        case ShaderKind_Pixel:   target = d3d11_pixelTarget;   break;
        case ShaderKind_Compute: target = d3d11_computeTarget; break;
    }
    
    return target;
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
