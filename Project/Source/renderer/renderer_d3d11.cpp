
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
    
    ID3D11DeviceContext* deviceContext;
    ID3D11Device* device;
    HRESULT res = D3D11CreateDevice(nullptr,
                                    D3D_DRIVER_TYPE_HARDWARE,
                                    nullptr,
                                    flags,
                                    featureLevels,
                                    ArrayCount(featureLevels),
                                    D3D11_SDK_VERSION,
                                    &device,
                                    nullptr,
                                    &deviceContext);
    
    assert(SUCCEEDED(res));
    
#ifndef NDEBUG
    // For debug builds enable debug break on API errors
    {
        ID3D11InfoQueue* info;
        device->QueryInterface(IID_ID3D11InfoQueue, (void**)&info);
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
        HRESULT res = device->QueryInterface(IID_IDXGIDevice, (void**)&dxgiDevice);
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
        
        res = factory->CreateSwapChainForHwnd((IUnknown*)device, win32.window, &swapchainDesc, nullptr, nullptr, &swapchain1);
        assert(SUCCEEDED(res));
        
        swapchain1->QueryInterface(IID_IDXGISwapChain2, (void**)&swapchain);
        
        // Disable Alt+Enter changing monitor resolution to match window size
        factory->MakeWindowAssociation(win32.window, DXGI_MWA_NO_ALT_ENTER);
        
        factory->Release();
        dxgiAdapter->Release();
        dxgiDevice->Release();
    }
    
    // Create Render Target View from swapchain
    ID3D11RenderTargetView* rtv;
    {
        ID3D11Texture2D* backBuffer = nullptr;
        res = swapchain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backBuffer);
        assert(SUCCEEDED(res));
        
        res = device->CreateRenderTargetView(backBuffer, nullptr, &rtv);
        assert(SUCCEEDED(res));
        
        backBuffer->Release();  // We don't need the back buffer anymore
    }
    
    r.device = device;
    r.deviceContext = deviceContext;
    r.swapchain = swapchain;
    r.swapchainWaitableObject = swapchain->GetFrameLatencyWaitableObject();
    assert(r.swapchainWaitableObject);
    r.rtv = rtv;
    
    // TODO: Initialization of buffers for immediate mode style rendering
    //ID3D11Buffer* 
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
    
    auto vec3Format = DXGI_FORMAT_R32G32B32_FLOAT;
    auto vec2Format = DXGI_FORMAT_R32G32_FLOAT;
    auto floatFormat = DXGI_FORMAT_R32_FLOAT;
    
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, vec3Format, 0, offsetof(Vertex, pos),      D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, vec3Format, 0, offsetof(Vertex, normal),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, vec2Format, 0, offsetof(Vertex, texCoord), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, vec3Format, 0, offsetof(Vertex, tangent),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    
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

R_Texture R_UploadTexture(String blob, u32 width, u32 height, u8 numChannels)
{
    return {};
}

R_Texture R_UploadCubemap(String top, String bottom, String left, String right, String front, String back, u32 width,
                          u32 height, u8 numChannels)
{
    return {};
}

R_Shader R_CreateDefaultShader(ShaderKind kind)
{
    String vertShader = StrLit(R""""(
                                                                                                                                                                                                                                                                                                                                                                                          struct VSInput
                                                                                                                                                                                                                                                                                                                                                               {
                                                                                                                                                                                                                                                                                                                                    float3 position : POSITION;
                                                                                                                                                                                                                                                                                                         }
                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                   struct VSOutput
                                                                                                                                                                                                                        {
                                                                                                                                                                                             float4 position : SV_POSITION;
                                                                                                                                                                  }
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  VSOutput VSMain(VSInput input)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       {
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            VSOutput output;
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 output.position = float4(input.position, 1.0f);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      return output;
                                                                                                                                                                                                                                                                                                                                                                                                                                                                           }
                                                                                                                                                                                                                                                                                                                                                                                                                                                )"""");
    
    String pixelShader = StrLit(R""""(
                                                                                                                                                                                                                                                                                                                                                #version 460 core
                                                                                                                                                                                                                                                                                                                    out vec4 fragColor;
                                                                                                                                                                                                                                                                                        void main()
                                                                                                                                                                                                                                                            {
                                                                                                                                                                                                                                fragColor = vec4(1.0f, 0.0f, 1.0f, 1.0f);
                                                                                                                                                                                                    }
                                                                                                                                                                        )"""");
    
    String computeShader = StrLit(R""""(
                                                                                                                                                                                                                                                #version 460 core
                                                                                                                                                                                                                  layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
                                                                                                                                                                                    void main() { }
                                                                                                                                                      )"""");
    
    // TODO: Maybe remove the d3dcompiler dependency with precompiled shaders and injected into the app binary?
    
    // TODO: continue working from here
    
    return {};
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

void R_SetPipeline(R_Pipeline pipeline)
{
    auto& r = renderer;
    r.deviceContext->VSSetShader(pipeline.vertShader,  nullptr, 0);
    r.deviceContext->PSSetShader(pipeline.pixelShader, nullptr, 0);
}

void R_SetUniforms(Slice<R_UniformValue> desc)
{
    
}

void R_SetFramebuffer(R_Framebuffer framebuffer)
{
    
}

void R_SetTexture(R_Texture texture, u32 slot)
{
    
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
