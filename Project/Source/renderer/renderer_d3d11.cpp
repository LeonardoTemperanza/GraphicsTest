
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
    samplerDesc.MipLODBias     = mipLODBias;  // Mipmap bias
    samplerDesc.MaxAnisotropy  = maxAnisotropy;  // Anisotropic filtering level
    samplerDesc.ComparisonFunc = compareFunc; // Comparison function for shadow mapping, etc.
    
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
                                    &r.deviceContext);
    
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
    IDXGISwapChain2* swapchain = nullptr;
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
        
        swapchain1->QueryInterface(IID_IDXGISwapChain2, (void**)&swapchain);
        
        // Disable Alt+Enter changing monitor resolution to match window size
        factory->MakeWindowAssociation(win32.window, DXGI_MWA_NO_ALT_ENTER);
        
        factory->Release();
        dxgiAdapter->Release();
        dxgiDevice->Release();
    }
    
    // Create Render Target View from swapchain
    {
        ID3D11Texture2D* backBuffer = nullptr;
        res = swapchain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backBuffer);
        assert(SUCCEEDED(res));
        
        res = r.device->CreateRenderTargetView(backBuffer, nullptr, &r.rtv);
        assert(SUCCEEDED(res));
        
        backBuffer->Release();  // We don't need the back buffer anymore
    }
    
    r.swapchain = swapchain;
    r.swapchainWaitableObject = swapchain->GetFrameLatencyWaitableObject();
    assert(r.swapchainWaitableObject);
    
    // TODO: Create depth buffer and other necessary things for main framebuffer
    {
        
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
    //ID3D11Buffer* 
    
    // TODO: Create commonly used samplers
    {
        r.samplers[R_SamplerDefault];
        r.samplers[R_SamplerShadow];
    }
}

void R_Cleanup()
{
    
}

Mat4 R_ConvertView2ProjMatrix(Mat4 mat)
{
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
    
    R_Texture res = {};
    res.kind = R_Tex2D;
    
    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
    switch(numChannels)
    {
        case 1:  format = DXGI_FORMAT_R8_UNORM;       break;
        case 2:  format = DXGI_FORMAT_R8G8_UNORM;     break;
        case 3:  format = DXGI_FORMAT_R8G8B8A8_UNORM; break; // DirectX typically pads to 4 channels
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
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS;
    texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    
    // Create the texture and upload data to GPU
    {
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = blob.ptr;
        initData.SysMemPitch = width * numChannels;
        
        HRESULT hr = r.device->CreateTexture2D(&texDesc, nullptr, &res.tex);
        assert(SUCCEEDED(hr));
        
        // Upload texture data to mipmap 0
        r.deviceContext->UpdateSubresource(res.tex, 0, nullptr, blob.ptr, width * numChannels, 0);
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
    r.deviceContext->GenerateMips(res.view);
    
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
        case ShaderKind_Compute:
        {
            ID3DBlob* shader = nullptr;
            ID3DBlob* errorBlob = nullptr;
            HRESULT hr = D3DCompile(computeShader.ptr, computeShader.len,
                                    "DefaultComputeShader",
                                    nullptr,  // Optional defines
                                    nullptr,  // Include handler (non needed)
                                    "main",
                                    "cs_5_0",
                                    0,  // Compilation Flags
                                    0,  // Effect compilation flags
                                    &shader,
                                    &errorBlob);
            
            if(FAILED(hr) && errorBlob)
            {
                Log("Default compute shader compilation failed: %s", (char*)errorBlob->GetBufferPointer());
                errorBlob->Release();
                return res;
            }
            
            r.device->CreateComputeShader(shader->GetBufferPointer(), shader->GetBufferSize(), nullptr, &res.computeShader);
            
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

R_Shader R_CompileShader(ShaderKind kind, ShaderInput input)
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
        case ShaderKind_Compute:
        {
            r.device->CreateComputeShader(input.d3d11Bytecode.ptr, input.d3d11Bytecode.len, nullptr, &res.computeShader);
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
            case ShaderKind_Compute: break;
        }
    }
    
    assert(res.vertShader);
    assert(res.pixelShader);
    return res;
}

R_Framebuffer R_DefaultFramebuffer()
{
    return {};
}

R_Framebuffer R_CreateFramebuffer(int width, int height, bool color, R_TextureFormat colorFormat, bool depth, bool stencil)
{
    return {};
}

void R_ResizeFramebuffer(R_Framebuffer framebuffer, int width, int height)
{
    
}

void R_DrawMesh(R_Mesh mesh)
{
    auto& r = renderer;
    
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    r.deviceContext->IASetInputLayout(r.staticMeshInputLayout);
    r.deviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    r.deviceContext->IASetVertexBuffers(0, 1, &mesh.vertBuf, &stride, &offset);
    r.deviceContext->IASetIndexBuffer(mesh.indexBuf, DXGI_FORMAT_R32_UINT, 0);
    r.deviceContext->DrawIndexed(mesh.numIndices, 0, 0);
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
    
}

void R_SetVertexShader(R_Shader shader)
{
    auto& r = renderer;
    assert(shader.kind == ShaderKind_Vertex);
    r.deviceContext->VSSetShader(shader.vertShader,  nullptr, 0);
}

void R_SetPixelShader(R_Shader shader)
{
    auto& r = renderer;
    assert(shader.kind == ShaderKind_Pixel);
    r.deviceContext->PSSetShader(shader.pixelShader, nullptr, 0);
}

void R_SetUniforms(Slice<R_UniformValue> desc)
{
    
}

void R_SetFramebuffer(R_Framebuffer framebuffer)
{
    
}

void R_SetTexture(R_Texture texture, u32 slot)
{
    auto& r = renderer;
}

void R_SetPerSceneData()
{
    
}

void R_SetPerFrameData(Mat4 world2View, Mat4 view2Proj, Vec3 viewPos)
{
    
}

void R_SetPerObjData(Mat4 model2World, Mat3 normalMat)
{
    
}

void R_ClearFrame(Vec4 color)
{
    auto& r = renderer;
    r.deviceContext->ClearRenderTargetView(r.rtv, (float*)&color);
}

void R_ClearFrameInt(int r, int g, int b, int a)
{
    
}

void R_ClearDepth()
{
    
}

void R_DepthTest(bool enable)
{
    //auto& r = renderer;
    //r.deviceContext->RSSetState();
}

void R_CullFace(bool enable)
{
    
}

void R_AlphaBlending(bool enable)
{
    
}

R_Texture R_GetFramebufferColorTexture(R_Framebuffer framebuffer)
{
    return {};
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
    
    r.deviceContext->OMSetRenderTargets(1, &r.rtv, nullptr);
}

void R_ResizeMainFramebufferIfNecessary(int width, int heigth)
{
    auto& r = renderer;
    
    
}

void R_SubmitFrame()
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
    ImGui_ImplDX11_Init(renderer.device, renderer.deviceContext);
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
