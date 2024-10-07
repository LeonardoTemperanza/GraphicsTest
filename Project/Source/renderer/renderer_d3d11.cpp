
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
    return {};
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

R_Shader R_MakeDefaultShader(ShaderKind kind)
{
    return {};
}

R_Mesh R_MakeDefaultMesh()
{
    return {};
}

R_Shader R_CompileShader(ShaderKind kind, String dxil, String vulkanSpirv, String glsl)
{
    return {};
}

R_Pipeline R_CreatePipeline(Slice<R_Shader> shaders)
{
    return {};
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
