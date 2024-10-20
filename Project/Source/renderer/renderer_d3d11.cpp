
#include "renderer_d3d11.h"

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>

#pragma warning(push)  // These includes have tons of warnings so we're disabling them
#pragma warning(disable : 4062)
#pragma warning(disable : 4061)
#include <d3d11.h>
#include <dxgi1_3.h>
#include <dxgidebug.h>
#include <d3d11sdklayers.h>
#include <d3dcompiler.h>
#pragma warning(pop)

// Dear imgui
#include "imgui/backends/imgui_impl_dx11.h"

// D3D utility functions
ID3D11SamplerState* D3D11_CreateSampler(D3D11_FILTER filter,
                                        D3D11_TEXTURE_ADDRESS_MODE wrapU, D3D11_TEXTURE_ADDRESS_MODE wrapV,
                                        float mipLODBias = 0.1f,
                                        UINT maxAnisotropy = 1,
                                        D3D11_COMPARISON_FUNC compareFunc = D3D11_COMPARISON_ALWAYS)
{
    auto& r = renderer;
    
    ID3D11SamplerState* res = nullptr;
    
    // Create a res description
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter         = filter;
    samplerDesc.AddressU       = wrapU;
    samplerDesc.AddressV       = wrapV;
    samplerDesc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MipLODBias     = mipLODBias;     // Mipmap bias
    samplerDesc.MaxAnisotropy  = maxAnisotropy;  // Anisotropic filtering level
    samplerDesc.ComparisonFunc = compareFunc;    // Comparison function for shadow mapping, etc.
    
    // Default border color (can be modified if needed)
    samplerDesc.BorderColor[0] = 0.0f;
    samplerDesc.BorderColor[1] = 0.0f;
    samplerDesc.BorderColor[2] = 0.0f;
    samplerDesc.BorderColor[3] = 0.0f;
    
    samplerDesc.MinLOD = 0.0f;  // Minimum LOD
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;  // Maximum LOD (allow all mip levels)
    
    // Create the sampler state object
    HRESULT hr = r.device->CreateSamplerState(&samplerDesc, &res);
    assert(SUCCEEDED(hr));
    return res;
}

DXGI_FORMAT D3D11_GetTextureFormat(R_TextureFormat format)
{
    switch(format)
    {
        case R_TexNone:  return DXGI_FORMAT_R8G8B8A8_UNORM;
        case R_TexR8:    return DXGI_FORMAT_R8_UNORM;
        case R_TexRG8:   return DXGI_FORMAT_R8G8_UNORM;
        case R_TexRGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case R_TexR8I:   return DXGI_FORMAT_R8_SINT;
        case R_TexR8UI:  return DXGI_FORMAT_R8_UINT;
        case R_TexR32I:  return DXGI_FORMAT_R32_SINT;
    }
    
    return DXGI_FORMAT_R8G8B8A8_UNORM;
}

void R_Init()
{
    auto& r = renderer;
    
    static const D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };
    
    // Create D3D11 device and context
    UINT flags = 0;
#ifndef NDEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    
    HRESULT res = D3D11CreateDevice(nullptr,
                                    D3D_DRIVER_TYPE_HARDWARE,
                                    nullptr,
                                    flags,
                                    featureLevels,
                                    ArrayCount(featureLevels),
                                    D3D11_SDK_VERSION,
                                    &r.device,
                                    nullptr,
                                    &r.context);
    
    assert(SUCCEEDED(res));
    
#ifndef NDEBUG
    // For debug builds enable debug break on API errors
    {
        ID3D11InfoQueue* info;
        r.device->QueryInterface(IID_ID3D11InfoQueue, (void**)&info);
        info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
        info->Release();
    }
    
    // Enable debug break for DXGI
    {
        IDXGIInfoQueue* dxgiInfo;
        res = DXGIGetDebugInterface1(0, IID_IDXGIInfoQueue, (void**)&dxgiInfo);
        assert(SUCCEEDED(res));
        dxgiInfo->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        dxgiInfo->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
        dxgiInfo->Release();
    }
#endif
    
    // Create DXGI swapchain
    {
        IDXGISwapChain1* swapchain1 = nullptr;
        
        // Get DXGI device from D3D11 device
        IDXGIDevice* dxgiDevice;
        HRESULT res = r.device->QueryInterface(IID_IDXGIDevice, (void**)&dxgiDevice);
        assert(SUCCEEDED(res));
        
        // Get DXGI adapter from DXGI device
        IDXGIAdapter* dxgiAdapter;
        res = dxgiDevice->GetAdapter(&dxgiAdapter);
        assert(SUCCEEDED(res));
        
        // Get DXGI factory from DXGI adapter
        IDXGIFactory2* factory;
        res = dxgiAdapter->GetParent(IID_IDXGIFactory3, (void**)&factory);
        assert(SUCCEEDED(res));
        
        DXGI_SWAP_CHAIN_DESC1 swapchainDesc =
        {
            .Width  = 0,  // (0 means "get it from hwnd")
            .Height = 0,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = { 1, 0 },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = 2,
            .Scaling = DXGI_SCALING_NONE,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
            .Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
        };
        
        res = factory->CreateSwapChainForHwnd((IUnknown*)r.device, win32.window, &swapchainDesc, nullptr, nullptr, &swapchain1);
        assert(SUCCEEDED(res));
        
        swapchain1->QueryInterface(IID_IDXGISwapChain2, (void**)&r.swapchain);
        r.swapchainWaitableObject = r.swapchain->GetFrameLatencyWaitableObject();
        assert(r.swapchainWaitableObject);
        
        // Disable Alt+Enter changing monitor resolution to match window size
        factory->MakeWindowAssociation(win32.window, DXGI_MWA_NO_ALT_ENTER);
        
        factory->Release();
        dxgiAdapter->Release();
        dxgiDevice->Release();
    }
    
    // TODO: Instead of creating all the states here we can just pass nullptr to OMSetXXX
    // Create blend state
    {
        r.blendDesc = {};
        r.blendDesc.RenderTarget[0] =
        {
            .BlendEnable = TRUE,
            .SrcBlend = D3D11_BLEND_SRC_ALPHA,
            .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
            .BlendOp = D3D11_BLEND_OP_ADD,
            .SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA,
            .DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
            .BlendOpAlpha = D3D11_BLEND_OP_ADD,
            .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
        };
        
        r.device->CreateBlendState(&r.blendDesc, &r.blendState);
        r.context->OMSetBlendState(r.blendState, nullptr, ~0U);
    }
    
    // Create rasterizer state
    {
        D3D11_RASTERIZER_DESC desc =
        {
            .FillMode = D3D11_FILL_SOLID,
            .CullMode = D3D11_CULL_BACK,
            .FrontCounterClockwise = true,
            .DepthClipEnable = true,
        };
        
        r.device->CreateRasterizerState(&desc, &r.rasterizerState);
        r.context->RSSetState(r.rasterizerState);
    }
    
    // Create depth state
    {
        D3D11_DEPTH_STENCIL_DESC desc =
        {
            .DepthEnable = true,
            .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
            .DepthFunc = D3D11_COMPARISON_LESS,
            .StencilEnable = false,
            .StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK,
            .StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK,
            // .FrontFace = ... 
            // .BackFace = ...
        };
        
        r.device->CreateDepthStencilState(&desc, &r.depthState);
        r.context->OMSetDepthStencilState(r.depthState, 0);
    }
    
    // Create Render Target View from swapchain
    {
        ID3D11Texture2D* backBuffer = nullptr;
        res = r.swapchain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backBuffer);
        assert(SUCCEEDED(res));
        
        res = r.device->CreateRenderTargetView(backBuffer, nullptr, &r.rtv);
        assert(SUCCEEDED(res));
        r.boundRtv = r.rtv;
    }
    
    // Create depth stencil buffer
    {
        int width, height;
        OS_GetClientAreaSize(&width, &height);
        
        D3D11_TEXTURE2D_DESC depthStencilDesc = {};
        depthStencilDesc.Width = width;
        depthStencilDesc.Height = height;
        depthStencilDesc.MipLevels = 1;
        depthStencilDesc.ArraySize = 1;
        depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;  // 24 bit depth, 8 bit stencil
        depthStencilDesc.SampleDesc.Count = 1;
        depthStencilDesc.SampleDesc.Quality = 0;
        depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
        depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depthStencilDesc.CPUAccessFlags = 0;
        depthStencilDesc.MiscFlags = 0;
        
        ID3D11Texture2D* depthStencilBuffer;
        HRESULT hr = r.device->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencilBuffer);
        assert(SUCCEEDED(hr));
        
        hr = r.device->CreateDepthStencilView(depthStencilBuffer, nullptr, &r.dsv);
        assert(SUCCEEDED(hr));
        
        r.boundDsv = r.dsv;
    }
    
    // Create commonly used input layouts
    // Static mesh
    {
        // NOTE: For some reason, D3D11 needs a shader bytecode when creating 
        // an input layout. So we just compile a ficitious shader just for this.
        
        String vertShader = StrLit("struct VSInput                                     "
                                   "{                                                  "
                                   "    float3 position : POSITION;                    "
                                   "    float3 normal   : NORMAL;                      "
                                   "    float2 texCoord : TEXCOORD;                    "
                                   "    float3 tangent  : TANGENT;                     "
                                   "};                                                 "
                                   "struct VSOutput { float4 position : SV_POSITION; };"
                                   "VSOutput main(VSInput input)                       "
                                   "{ VSOutput output;                                 "
                                   " output.position = float4(input.position, 1.0f);   "
                                   " return output; }                                  ");
        
        ID3DBlob* shader = nullptr;
        ID3DBlob* errorBlob = nullptr;
        HRESULT hr = D3DCompile(vertShader.ptr, vertShader.len,
                                "DefaultVertShader",
                                nullptr,  // Optional defines
                                nullptr,  // Include handler (non needed)
                                "main",
                                "vs_5_0",
                                0,  // Compilation Flags
                                0,  // Effect compilation flags
                                &shader,
                                &errorBlob);
        assert(SUCCEEDED(hr) && "Fictitious shader compilation failed!");
        defer { shader->Release(); };
        
        D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[] =
        {
            { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        
        r.device->CreateInputLayout(inputLayoutDesc, ArrayCount(inputLayoutDesc),
                                    shader->GetBufferPointer(), shader->GetBufferSize(), 
                                    &r.staticMeshInputLayout);
        
    }
    
    // TODO: Skinned mesh
    {
        
    }
    
    // TODO: Basic mesh
    {
        
    }
    
    // TODO: Initialization of buffers for immediate mode style rendering
    
    // Create commonly used samplers
    {
        r.samplers[R_SamplerDefault] = D3D11_CreateSampler(D3D11_FILTER_MIN_MAG_MIP_LINEAR,
                                                           D3D11_TEXTURE_ADDRESS_WRAP,
                                                           D3D11_TEXTURE_ADDRESS_WRAP);
        // TODO: Shadow sampler
        //r.samplers[R_SamplerShadow];
    }
    
    // Create and bind constant buffers
    {
        constexpr int perFrameBindPoint = 1;
        constexpr int perObjBindPoint = 2;
        
        // Per Frame
        {
            D3D11_BUFFER_DESC bufferDesc = {};
            bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
            bufferDesc.ByteWidth = 144;  // TODO: Put actual size of cbuffer here
            bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            
            HRESULT hr = r.device->CreateBuffer(&bufferDesc, nullptr, &r.perFrameBuf);
            assert(SUCCEEDED(hr));
            
            r.context->VSSetConstantBuffers(perFrameBindPoint, 1, &r.perFrameBuf);
            r.context->PSSetConstantBuffers(perFrameBindPoint, 1, &r.perFrameBuf);
        }
        
        // Per obj
        {
            D3D11_BUFFER_DESC bufferDesc = {};
            bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
            bufferDesc.ByteWidth = 144;  // TODO: Put actual size of cbuffer here
            bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            
            HRESULT hr = r.device->CreateBuffer(&bufferDesc, nullptr, &r.perObjBuf);
            assert(SUCCEEDED(hr));
            
            r.context->VSSetConstantBuffers(perObjBindPoint, 1, &r.perObjBuf);
            r.context->PSSetConstantBuffers(perObjBindPoint, 1, &r.perObjBuf);
        }
    }
}

void R_Cleanup()
{
    
}

Mat4 R_ConvertClipSpace(Mat4 mat)
{
    // I calculate the matrix for [-1, 1] in all axes with x pointing right,
    // y pointing up, z pointing into the screen. DX is the same thing except it has
    // z pointing outwards from the screen, and has range [0, 1] in the z axis,
    // so we need to halve our value
    
    // First flip the z axis
    mat.m13 *= -1;
    mat.m23 *= -1;
    mat.m33 *= -1;
    mat.m43 = 1;
    
    // Convert z from [-1, 1] to [0, 1]
    // (multiply with identity matrix but with m33 and m34 = 0.5)
    mat.m31 = 0.5f * mat.m31 + 0.5f * mat.m41;
    mat.m32 = 0.5f * mat.m32 + 0.5f * mat.m42;
    mat.m33 = 0.5f * mat.m33 + 0.5f * mat.m43;
    mat.m34 = 0.5f * mat.m34 + 0.5f * mat.m44;
    
    return mat;
}

R_Mesh R_UploadMesh(Slice<Vertex> verts, Slice<s32> indices)
{
    auto& r = renderer;
    
    R_Mesh res = {};
    res.numIndices = (u32)indices.len;
    
    // Vertex buffer
    {
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = (u32)(sizeof(verts[0]) * verts.len);
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        
        D3D11_SUBRESOURCE_DATA vertData = {};
        vertData.pSysMem = verts.ptr;
        
        HRESULT hr = r.device->CreateBuffer(&bufferDesc, &vertData, &res.vertBuf);
        assert(SUCCEEDED(hr));
    }
    
    // Index buffer
    {
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = (u32)(sizeof(verts[0]) * verts.len);
        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        
        D3D11_SUBRESOURCE_DATA indexData = {};
        indexData.pSysMem = indices.ptr;
        
        HRESULT hr = r.device->CreateBuffer(&bufferDesc, &indexData, &res.indexBuf);
        assert(SUCCEEDED(hr));
    }
    
    return res;
}

R_Mesh R_UploadSkinnedMesh(Slice<AnimVert> verts, Slice<s32> indices)
{
    return {};
}

R_Mesh R_UploadBasicMesh(Slice<Vec3> verts, Slice<Vec3> normals, Slice<s32> indices)
{
    return {};
}

R_Texture R_UploadTexture(String blob, u32 width, u32 height, u8 numChannels)
{
    auto& r = renderer;
    assert(numChannels >= 1 && numChannels <= 4);
    assert(blob.ptr);
    
    R_Texture res = {};
    res.kind = R_Tex2D;
    
    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
    switch(numChannels)
    {
        case 1:  format = DXGI_FORMAT_R8_UNORM;       break;
        case 2:  format = DXGI_FORMAT_R8G8_UNORM;     break;
        case 3:  format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
        case 4:  format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
        default: assert(false && "Unsupported channel count."); break;
    }
    
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width  = width;
    texDesc.Height = height;
    texDesc.MipLevels = 0;  // Let D3D generate mipmaps
    texDesc.ArraySize = 1;
    texDesc.Format = format;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    
    // Create the texture and upload data to GPU
    {
        HRESULT hr = r.device->CreateTexture2D(&texDesc, nullptr, &res.tex);
        assert(SUCCEEDED(hr));
        
        r.context->UpdateSubresource(res.tex, 0, nullptr, blob.ptr, width * 4, 0);
    }
    
    // Create a Shader Resource View
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = texDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = -1; // Use all available mipmap levels
        
        HRESULT hr = r.device->CreateShaderResourceView(res.tex, &srvDesc, &res.view);
        assert(SUCCEEDED(hr));
    }
    
    // Generate mipmaps
    r.context->GenerateMips(res.view);
    
    return res;
}

R_Texture R_UploadCubemap(String top, String bottom, String left, String right, String front, String back, u32 width,
                          u32 height, u8 numChannels)
{
    return {};
}

R_Shader R_CreateDefaultShader(ShaderKind kind)
{
    auto& r = renderer;
    
    String vertShader = StrLit("struct VSInput  { float3 position : POSITION; };   "
                               "struct VSOutput { float4 position : SV_POSITION; };"
                               "VSOutput main(VSInput input)                       "
                               "{ VSOutput output;                                 "
                               " output.position = float4(input.position, 1.0f);   "
                               " return output; } ");
    
    String pixelShader = StrLit("float4 main() : SV_TARGET                 "
                                "{ return float4(1.0f, 0.0f, 1.0f, 1.0f); }");
    
    String computeShader = StrLit("[numthreads(8, 8, 1)]                           "
                                  "void main(uint3 DTid : SV_DispatchThreadID) { } ");
    
    R_Shader res = {};
    res.kind = kind;
    
    switch(kind)
    {
        case ShaderKind_None: break;
        case ShaderKind_Count: break;
        case ShaderKind_Vertex:
        {
            ID3DBlob* shader = nullptr;
            ID3DBlob* errorBlob = nullptr;
            HRESULT hr = D3DCompile(vertShader.ptr, vertShader.len,
                                    "DefaultVertShader",
                                    nullptr,  // Optional defines
                                    nullptr,  // Include handler (non needed)
                                    "main",
                                    "vs_5_0",
                                    0,  // Compilation Flags
                                    0,  // Effect compilation flags
                                    &shader,
                                    &errorBlob);
            
            if(FAILED(hr) && errorBlob)
            {
                Log("Default vertex shader compilation failed: %s", (char*)errorBlob->GetBufferPointer());
                errorBlob->Release();
                return res;
            }
            
            r.device->CreateVertexShader(shader->GetBufferPointer(), shader->GetBufferSize(), nullptr, &res.vertShader);
            
            shader->Release();
            break;
        }
        case ShaderKind_Pixel:
        {
            ID3DBlob* shader = nullptr;
            ID3DBlob* errorBlob = nullptr;
            HRESULT hr = D3DCompile(pixelShader.ptr, pixelShader.len,
                                    "DefaultPixelShader",
                                    nullptr,  // Optional defines
                                    nullptr,  // Include handler (non needed)
                                    "main",
                                    "ps_5_0",
                                    0,  // Compilation Flags
                                    0,  // Effect compilation flags
                                    &shader,
                                    &errorBlob);
            
            if(FAILED(hr) && errorBlob)
            {
                Log("Default pixel shader compilation failed: %s", (char*)errorBlob->GetBufferPointer());
                errorBlob->Release();
                return res;
            }
            
            r.device->CreatePixelShader(shader->GetBufferPointer(), shader->GetBufferSize(), nullptr, &res.pixelShader);
            
            shader->Release();
            break;
        }
    }
    
    return res;
}

R_Mesh R_CreateDefaultMesh()
{
    return {};
}

// TODO: This stuff should really be moved to the shader importer
Array<ShaderValType> D3D11_GetVarTypesInCBuffer(ID3D11ShaderReflectionConstantBuffer* cbuf)
{
    assert(cbuf);
    
    Array<ShaderValType> res = {};
    
    D3D11_SHADER_BUFFER_DESC bufferDesc;
    HRESULT hr = cbuf->GetDesc(&bufferDesc);
    assert(SUCCEEDED(hr));
    
    for(UINT i = 0; i < bufferDesc.Variables; ++i)
    {
        auto variable = cbuf->GetVariableByIndex(i);
        
        // Get variable description
        D3D11_SHADER_VARIABLE_DESC varDesc;
        variable->GetDesc(&varDesc);
        
        // Get variable type description
        ID3D11ShaderReflectionType* type = variable->GetType();
        D3D11_SHADER_TYPE_DESC typeDesc;
        HRESULT hr = type->GetDesc(&typeDesc);
        assert(hr);
        
        ShaderValType toAppend = Uniform_None;
        switch(typeDesc.Class)
        {
            default: Log("Unrecognized type in %s cbuffer", bufferDesc.Name); break;
            case D3D_SVC_SCALAR:
            {
                switch(typeDesc.Type)
                {
                    default: Log("Unrecognized type in %s cbuffer", bufferDesc.Name); break;
                    case D3D_SVT_FLOAT: toAppend = Uniform_Float; break;
                    case D3D_SVT_UINT:  toAppend = Uniform_UInt;  break;
                    case D3D_SVT_INT:   toAppend = Uniform_Int; break;
                }
                
                break;
            }
            case D3D_SVC_VECTOR:
            {
                assert(typeDesc.Rows == 1);
                
                if(typeDesc.Type != D3D_SVT_FLOAT)
                {
                    Log("Unrecognized type in %s cbuffer", bufferDesc.Name);
                    break;
                }
                
                switch(typeDesc.Columns)
                {
                    default: Log("Unrecognized type in %s cbuffer", bufferDesc.Name);
                    case 3: toAppend = Uniform_Vec3; break;
                    case 4: toAppend = Uniform_Vec4; break;
                }
                break;
            }
        }
        
        Append(&res, toAppend);
    }
    
    return res;
}

ID3D11Buffer* D3D11_CreateBufferForCBuf(ID3D11ShaderReflectionConstantBuffer* cbuf)
{
    auto& r = renderer;
    
    D3D11_SHADER_BUFFER_DESC reflBufferDesc;
    HRESULT hr = cbuf->GetDesc(&reflBufferDesc);
    assert(SUCCEEDED(hr));
    
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = reflBufferDesc.Size;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    ID3D11Buffer* res = nullptr;
    hr = r.device->CreateBuffer(&bufferDesc, nullptr, &res);
    assert(SUCCEEDED(hr));
    return res;
}

R_Shader R_CreateShader(ShaderKind kind, ShaderInput input)
{
    auto& r = renderer;
    
    R_Shader res = {};
    res.kind = kind;
    switch(kind)
    {
        case ShaderKind_None: break;
        case ShaderKind_Count: break;
        case ShaderKind_Vertex:
        {
            r.device->CreateVertexShader(input.d3d11Bytecode.ptr, input.d3d11Bytecode.len, nullptr, &res.vertShader);
            break;
        }
        case ShaderKind_Pixel:
        {
            r.device->CreatePixelShader(input.d3d11Bytecode.ptr, input.d3d11Bytecode.len, nullptr, &res.pixelShader);
            break;
        }
    }
    
    // TODO: This stuff should really be moved to the importer
    ID3D11ShaderReflection* reflection = nullptr;
    HRESULT hr = D3DReflect(input.d3d11Bytecode.ptr, input.d3d11Bytecode.len, IID_ID3D11ShaderReflection, (void**)&reflection);
    assert(SUCCEEDED(hr));
    
    D3D11_SHADER_DESC shaderDesc;
    hr = reflection->GetDesc(&shaderDesc);
    
    // Get material and code constants
    ID3D11ShaderReflectionConstantBuffer* materialCBuf = nullptr;
    ID3D11ShaderReflectionConstantBuffer* codeCBuf = nullptr;
    for(UINT i = 0; i < shaderDesc.BoundResources; ++i)
    {
        D3D11_SHADER_INPUT_BIND_DESC bindDesc;
        HRESULT hr = reflection->GetResourceBindingDesc(i, &bindDesc);
        assert(SUCCEEDED(hr));
        
        if(bindDesc.Type == D3D_SIT_CBUFFER)
        {
            auto slot = bindDesc.BindPoint;
            if(slot == MaterialConstants)
                materialCBuf = reflection->GetConstantBufferByName(bindDesc.Name);
            else if(slot == CodeConstants)
                codeCBuf = reflection->GetConstantBufferByName(bindDesc.Name);
        }
    }
    
    if(materialCBuf)
    {
        res.materialConstantTypes = D3D11_GetVarTypesInCBuffer(materialCBuf);
        res.materialConstants = D3D11_CreateBufferForCBuf(materialCBuf);
    }
    
    if(codeCBuf)
    {
        res.codeConstantTypes = D3D11_GetVarTypesInCBuffer(codeCBuf);
        res.codeConstants = D3D11_CreateBufferForCBuf(codeCBuf);
    }
    
    // Get number of textures and samplers
    for(UINT i = 0; i < shaderDesc.BoundResources; ++i)
    {
        D3D11_SHADER_INPUT_BIND_DESC bindDesc;
        HRESULT hr = reflection->GetResourceBindingDesc(i, &bindDesc);
        assert(SUCCEEDED(hr));
        
        if(bindDesc.Type == D3D_SIT_TEXTURE)
        {
            auto slot = bindDesc.BindPoint;
            bool isMaterial = slot >= MaterialTex0;
            bool isCode     = slot >= CodeTex0 && !isMaterial;
            if(isMaterial)
                ++res.materialTexturesCount;
            if(isCode)
                ++res.codeTexturesCount;
        }
        else if(bindDesc.Type == D3D_SIT_SAMPLER)
        {
            auto slot = bindDesc.BindPoint;
            bool isMaterial = slot >= MaterialSampler0;
            bool isCode     = slot >= CodeSampler0 && !isMaterial;
            if(isCode)
                ++res.codeSamplersCount;
        }
    }
    
    return res;
}

R_Pipeline R_CreatePipeline(Slice<R_Shader> shaders)
{
    R_Pipeline res = {};
    for(int i = 0; i < shaders.len; ++i)
    {
        switch(shaders[i].kind)
        {
            case ShaderKind_None: break;
            case ShaderKind_Count: break;
            case ShaderKind_Vertex:
            {
                res.vertShader = shaders[i].vertShader;
                break;
            }
            case ShaderKind_Pixel:
            {
                res.pixelShader = shaders[i].pixelShader;
                break;
            }
        }
    }
    
    assert(res.vertShader);
    assert(res.pixelShader);
    return res;
}

R_Framebuffer R_DefaultFramebuffer()
{
    auto& r = renderer;
    R_Framebuffer res = {};
    res.color = r.backbufferColor;
    res.depthStencil = r.backbufferDepthStencil;
    res.rtv = r.rtv;
    res.dsv = r.dsv;
    return res;
}

R_Framebuffer R_CreateFramebuffer(int width, int height, bool color, R_TextureFormat colorFormat, bool depthStencil)
{
    width  = max(width, 1);
    height = max(height, 1);
    
    auto& r = renderer;
    R_Framebuffer res = {};
    res.width = width;
    res.height = height;
    res.format = colorFormat;
    
    if(color)
    {
        D3D11_TEXTURE2D_DESC textureDesc;
        ZeroMemory(&textureDesc, sizeof(textureDesc));
        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = D3D11_GetTextureFormat(colorFormat);
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        
        r.device->CreateTexture2D(&textureDesc, nullptr, &res.color);
        r.device->CreateRenderTargetView(res.color, nullptr, &res.rtv);
    }
    
    if(depthStencil)
    {
        D3D11_TEXTURE2D_DESC depthStencilDesc;
        ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
        depthStencilDesc.Width = width;
        depthStencilDesc.Height = height;
        depthStencilDesc.MipLevels = 1;
        depthStencilDesc.ArraySize = 1;
        depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthStencilDesc.SampleDesc.Count = 1;
        depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
        depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        
        r.device->CreateTexture2D(&depthStencilDesc, nullptr, &res.depthStencil);
        r.device->CreateDepthStencilView(res.depthStencil, nullptr, &res.dsv);
    }
    
    return res;
}

void R_ResizeFramebuffer(R_Framebuffer* framebuffer, int width, int height)
{
    if(width == framebuffer->width && height == framebuffer->height) return;
    assert(framebuffer->rtv != renderer.rtv);
    assert(framebuffer->dsv != renderer.dsv);
    
    bool hasColor = framebuffer->color;
    bool hasDepthStencil = framebuffer->depthStencil;
    
    if(framebuffer->rtv)          framebuffer->rtv->Release();
    if(framebuffer->dsv)          framebuffer->dsv->Release();
    if(framebuffer->color)        framebuffer->color->Release();
    if(framebuffer->depthStencil) framebuffer->depthStencil->Release();
    
    *framebuffer = R_CreateFramebuffer(width, height, hasColor, framebuffer->format, hasDepthStencil);
}

void R_DrawMesh(R_Mesh mesh)
{
    auto& r = renderer;
    
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    r.context->IASetInputLayout(r.staticMeshInputLayout);
    r.context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    r.context->IASetVertexBuffers(0, 1, &mesh.vertBuf, &stride, &offset);
    r.context->IASetIndexBuffer(mesh.indexBuf, DXGI_FORMAT_R32_UINT, 0);
    r.context->DrawIndexed(mesh.numIndices, 0, 0);
}

void R_DrawSphere(Vec3 center, float radius)
{
    
}

void R_DrawInvertedSphere(Vec3 center, float radius)
{
    
}

void R_DrawCone(Vec3 baseCenter, Vec3 dir, float radius, float length)
{
    
}

void R_DrawCylinder(Vec3 center, Vec3 dir, float radius, float height)
{
    
}

// Counter clockwise is assumed. Both faces face the same way
void R_DrawQuad(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4)
{
    
}

void R_DrawFullscreenQuad()
{
    
}

void R_SetViewport(int width, int height)
{
    auto& r = renderer;
    
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width    = (float)width;
    viewport.Height   = (float)height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    
    r.context->RSSetViewports(1, &viewport);
}

void R_SetVertexShader(R_Shader shader)
{
    auto& r = renderer;
    assert(shader.kind == ShaderKind_Vertex);
    r.context->VSSetShader(shader.vertShader,  nullptr, 0);
}

void R_SetPixelShader(R_Shader shader)
{
    auto& r = renderer;
    assert(shader.kind == ShaderKind_Pixel);
    r.context->PSSetShader(shader.pixelShader, nullptr, 0);
}

void R_SetCodeConstants_(R_Shader shader, Slice<R_UniformValue> desc, const char* callFile, int callLine)
{
    auto& r = renderer;
    
#ifdef Development
    // Check correctness
    if(desc.len != shader.codeConstantTypes.len)
    {
        Log("Attempted to set incorrect number of constants to shader at file: %s, line: %d. Expecting %d values, but %d were given.", callFile, callLine, (int)shader.codeConstantTypes.len, (int)desc.len);
        return;
    }
    
    for(int i = 0; i < desc.len; ++i)
    {
        if(desc[i].type != shader.codeConstantTypes[i])
        {
            Log("Attempted to set incorrect type of constant to shader at file: %s, line: %d. Value n.%d is of incorrect type.", callFile, callLine, i);
            return;
        }
    }
#endif
    
    ScratchArena scratch;
    Slice<uchar> buffer = MakeUniformBufferStd140(desc, scratch);
    
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = r.context->Map(shader.codeConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    assert(SUCCEEDED(hr));
    memcpy(mapped.pData, buffer.ptr, buffer.len);
    r.context->Unmap(shader.codeConstants, 0);
}

// TODO: Maybe this should be moved elsewhere? Seems general enough
bool R_CheckMaterial(Material* mat, String matName)
{
    R_Shader shader = mat->pixelShader;
    
    // Check constants
    if(mat->uniforms.len != shader.codeConstantTypes.len)
    {
        Log("Error in material %.*s: incorrect number of constants. Expecting %d values, but %d were given.", StrPrintf(matName), shader.codeConstantTypes.len, mat->uniforms.len);
        return false;
    }
    
    for(int i = 0; i < mat->uniforms.len; ++i)
    {
        if(mat->uniforms[i].type != shader.codeConstantTypes[i])
        {
            Log("Error in material %.*s: incorrect type of constant at index %d.", StrPrintf(matName), i);
            return false;
        }
    }
    
    // Check textures
    if(mat->textures.len != shader.materialTexturesCount)
    {
        Log("Error in material %.*s: incorrect number of textures. Expecting %d textures, but %d were given.", StrPrintf(matName), shader.materialTexturesCount, mat->textures.len);
        return false;
    }
    
    return true;
}

void R_SetMaterialConstants(R_Shader shader, Slice<R_UniformValue> desc)
{
    auto& r = renderer;
    
    if(desc.len <= 0) return;
    
    if(!shader.materialConstants)
    {
        Log("Error: Attempting to set material contants when the shader does not have such cbuffer.");
        return;
    }
    
    // We assume that the material is correct
    
    ScratchArena scratch;
    Slice<uchar> buffer = MakeUniformBufferStd140(desc, scratch);
    
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = r.context->Map(shader.materialConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    assert(SUCCEEDED(hr));
    memcpy(mapped.pData, buffer.ptr, buffer.len);
    r.context->Unmap(shader.materialConstants, 0);
}

void R_SetFramebuffer(R_Framebuffer framebuffer)
{
    auto& r = renderer;
    r.context->OMSetRenderTargets(1, &framebuffer.rtv, framebuffer.dsv);
    r.boundRtv = framebuffer.rtv;
    r.boundDsv = framebuffer.dsv;
}

void R_SetTexture(R_Texture texture, ShaderKind kind, TextureSlot slot)
{
    auto& r = renderer;
    //assert(texture.view);
    
    // TODO: @speed @cleanup Just make different functions here
    switch(kind)
    {
        case ShaderKind_None: break;
        case ShaderKind_Count: break;
        case ShaderKind_Vertex:
        {
            r.context->VSSetShaderResources(slot, 1, &texture.view);
            break;
        }
        case ShaderKind_Pixel:
        {
            r.context->PSSetShaderResources(slot, 1, &texture.view);
            break;
        }
    }
}

void R_SetSampler(R_SamplerKind samplerKind, ShaderKind kind, SamplerSlot slot)
{
    auto& r = renderer;
    assert(samplerKind >= 0 && samplerKind < R_SamplerCount);
    
    // TODO: @speed @cleanup Just make different functions here
    switch(kind)
    {
        case ShaderKind_None: break;
        case ShaderKind_Count: break;
        case ShaderKind_Vertex:
        {
            r.context->VSSetSamplers(slot, 1, &r.samplers[samplerKind]);
            break;
        }
        case ShaderKind_Pixel:
        {
            r.context->PSSetSamplers(slot, 1, &r.samplers[samplerKind]);
            break;
        }
    }
}

void R_SetPerSceneData()
{
    
}

void R_SetPerFrameData(Mat4 world2View, Mat4 view2Proj, Vec3 viewPos)
{
    auto& r = renderer;
    
    struct alignas(16) PerFrameDataStd140
    {
        Mat4 world2View;
        Mat4 view2Proj;
        Vec3 viewPos;
        float padding;
    } perFrameStd140;
    
    // Fill in the struct with the correct layout
    perFrameStd140.world2View = world2View;
    perFrameStd140.view2Proj  = view2Proj;
    perFrameStd140.viewPos    = viewPos;
    perFrameStd140.padding    = 0.0f;
    
    // Write the new data
    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    HRESULT hr = r.context->Map(r.perFrameBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    assert(SUCCEEDED(hr));
    
    memcpy(mappedResource.pData, &perFrameStd140, sizeof(PerFrameDataStd140));
    
    r.context->Unmap(r.perFrameBuf, 0);
}

void R_SetPerObjData(Mat4 model2World, Mat3 normalMat)
{
    auto& r = renderer;
    
    struct alignas(16) PerObjDataStd140
    {
        Mat4 model2World;
        float normalMat[3][4];
    } perObjStd140;
    
    // Fill in the struct with the correct layout
    perObjStd140.model2World = model2World;
    
    // TODO: Implement Mat3x4 and just assign it here with "="
    for(int i = 0; i < 3; ++i)
    {
        for(int j = 0; j < 3; ++j)
            perObjStd140.normalMat[i][j] = normalMat.m[i][j];
    }
    
    // Write the new data
    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    HRESULT hr = r.context->Map(r.perObjBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    assert(SUCCEEDED(hr));
    
    memcpy(mappedResource.pData, &perObjStd140, sizeof(PerObjDataStd140));
    
    r.context->Unmap(r.perObjBuf, 0);
}

void R_ClearFrame(Vec4 color)
{
    auto& r = renderer;
    r.context->ClearRenderTargetView(r.boundRtv, (float*)&color);
    r.context->ClearDepthStencilView(r.boundDsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void R_ClearFrameInt(int r, int g, int b, int a)
{
    // TODO: Check that the format of the bound rtv is correct
    
    int vec[] = {r, g, b, a};
    //renderer.context->ClearRenderTargetView(r.boundRtv, (int*)&vec);
    //renderer.context->ClearDepthStencilView(r.boundDsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void R_ClearFrameUInt(u32 r, u32 g, u32 b, u32 a)
{
    u32 vec[] = {r, g, b, a};
    //renderer.context->ClearRenderTargetView(r.boundRtv, (float*)&vec);
}

void R_ClearDepth()
{
    auto& r = renderer;
    r.context->ClearDepthStencilView(r.dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void R_DepthTest(bool enable)
{
    auto& r = renderer;
    //r.context->RSSetState();
}

void R_CullFace(bool enable)
{
    
}

void R_AlphaBlending(bool enable)
{
    auto& r = renderer;
    
    if(r.blendState) r.blendState->Release();
    
    r.blendDesc.RenderTarget[0].BlendEnable = enable;
    r.device->CreateBlendState(&r.blendDesc, &r.blendState);
    r.context->OMSetBlendState(r.blendState, nullptr, ~0U);
}

R_Texture R_GetFramebufferColorTexture(R_Framebuffer* framebuffer)
{
    auto& r = renderer;
    
    R_Texture res = {};
    res.kind = R_Tex2D;
    res.tex  = framebuffer->color;
    
    if(!framebuffer->colorShaderInput)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = D3D11_GetTextureFormat(framebuffer->format);
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = -1; // Use all available mipmap levels
        r.device->CreateShaderResourceView(res.tex, &srvDesc, &framebuffer->colorShaderInput);
    }
    
    res.view = framebuffer->colorShaderInput;
    return res;
}

int R_ReadIntPixelFromFramebuffer(int x, int y)
{
    return {};
}

Vec4 R_ReadPixelFromFramebuffer(int x, int y)
{
    return {};
}

void R_WaitLastFrameAndBeginCurrentFrame()
{
    auto& r = renderer;
    
    // This should stall the CPU until all GPU commands have finished
    DWORD waitRes = WaitForSingleObjectEx(r.swapchainWaitableObject, 2000, false);
    switch(waitRes)
    {
        case WAIT_ABANDONED:
        {
            OS_FatalError("D3D11 Error during Swapchain wait. State: Abandoned.");
            break;
        }
        case WAIT_IO_COMPLETION:
        {
            OS_FatalError("D3D11 Error during Swapchain wait. State: IO-Completion.");
            break;
        };
        case WAIT_OBJECT_0:
        {
            // Ok!
            break;
        }
        case WAIT_TIMEOUT:
        {
            OS_FatalError("D3D11 Error during Swapchain wait. State: Timeout.");
            break;
        }
        case WAIT_FAILED:
        {
            OS_FatalError("D3D11 Error during Swapchain wait. State: Failed.");
            break;
        }
    }
    
    // Present call unbinds the render targets, so we need to rebind it
    r.context->OMSetRenderTargets(1, &r.rtv, r.dsv);
}

void R_ResizeSwapchainIfNecessary(int width, int heigth)
{
    auto& r = renderer;
    
    // Call:
    // context->ClearState();
    // rtv->Release();
    // get the backbuffer from the swapchain and release the backbuffer texture
    // swapchain->ResizeBuffers();
}

void R_PresentFrame()
{
    auto& r = renderer;
    
    bool vsync = true;
    HRESULT hr = r.swapchain->Present(vsync ? 1 : 0, 0);
    if(hr == DXGI_STATUS_OCCLUDED)
    {
        // window is minimized, cannot vsync - instead sleep a bit
        if(vsync)
        {
            Sleep(10);
        }
    }
    else if(FAILED(hr))
    {
        OS_FatalError("Failed to present swap chain! Device lost?");
    }
}

// Libraries
void R_DearImguiInit()
{
    ImGui_ImplDX11_Init(renderer.device, renderer.context);
}

void R_DearImguiBeginFrame()
{
    ImGui_ImplDX11_NewFrame();
}

void R_DearImguiRender()
{
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void R_DearImguiShutdown()
{
    ImGui_ImplDX11_Shutdown();
}
