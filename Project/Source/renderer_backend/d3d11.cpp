
#include "renderer_backend/generic.h"
#include "imgui/backends/imgui_impl_dx11.h"

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>

static Renderer renderer;

// NOTE: Overall policy here is we try to warn the user about things
// when it's really useful and needed, but most of the time we rely on the
// built-in D3D11 debugging utilities.

// D3D11 Utils
#define SafeRelease(obj) do { if(obj) { obj->Release(); obj = nullptr; } } while(0)
static u32 D3D11_FormatGetPixelSize(R_TextureFormat format);
static u32 D3D11_GetBufferBindFlags(R_BufferFlags flags);
static u32 D3D11_GetTexBindFlags(R_TextureUsage usage);
static DXGI_FORMAT D3D11_ConvertTextureFormat(R_TextureFormat format);
static D3D11_USAGE D3D11_ConvertTextureMutability(R_TextureMutability mut);
static u32 D3D11_GetTextureCPUAccess(R_TextureMutability mut);
static u32 D3D11_ConvertSamplerFilter(R_SamplerFilter min, R_SamplerFilter mag);
static u32 D3D11_ConvertSamplerWrap(R_SamplerWrap wrap);
static D3D11_INPUT_ELEMENT_DESC D3D11_ConvertVertAttrib(R_VertAttrib attrib);
static String D3D11_BuildDummyShaderForInputLayout(Slice<R_VertAttrib> attribs, Arena* dst);
static D3D11_RASTERIZER_DESC D3D11_ConvertRasterizerDesc(R_RasterizerDesc desc);
static D3D11_DEPTH_STENCIL_DESC D3D11_ConvertDepthStateDesc(R_DepthDesc desc);

// Resources

// Buffers
R_Buffer R_BufferAlloc(R_BufferFlags flags, u32 stride, u64 size, void* initData)
{
    auto& r = renderer;
    
    R_Buffer res = {};
    res.flags  = flags;
    res.stride = stride;
    res.size   = size;
    
    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = initData;
    
    D3D11_BUFFER_DESC desc = {0};
    desc.ByteWidth = (u32)size;
    desc.Usage = res.flags & BufferFlag_Dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_GetBufferBindFlags(res.flags);
    desc.CPUAccessFlags = res.flags & BufferFlag_Dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
    desc.StructureByteStride = res.stride;
    
    HRESULT hr = r.device->CreateBuffer(&desc, initData ? &sd : nullptr, &res.handle);
    assert(SUCCEEDED(hr));
    
    return res;
}

void R_BufferUpdate(R_Buffer* b, u64 offset, u64 size, void* data)
{
    if(size <= 0) return;
    if(!data) return;
    
    auto& r = renderer;
    
    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    r.context->Map((ID3D11Resource*)b->handle, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	memmove(((u8*)mappedResource.pData) + offset, data, size);
	r.context->Unmap((ID3D11Resource*)b->handle, 0);
}

void R_BufferFree(R_Buffer* b)
{
    SafeRelease(b->handle);
}

// Shaders
R_Shader R_ShaderAlloc(R_ShaderInput input, R_ShaderType type)
{
    auto& r = renderer;
    
    R_Shader res = {};
    res.type = type;
    switch(res.type)
    {
        case ShaderType_Null: break;
        case ShaderType_Count: break;
        case ShaderType_Vertex:
        {
            r.device->CreateVertexShader(input.d3d11Bytecode.ptr, input.d3d11Bytecode.len, nullptr, &res.vs);
            break;
        }
        case ShaderType_Pixel:
        {
            r.device->CreatePixelShader(input.d3d11Bytecode.ptr, input.d3d11Bytecode.len, nullptr, &res.ps);
            break;
        }
    }
    
    return res;
}

void R_ShaderBind(R_Shader* shader)
{
    auto& r = renderer;
    
    switch(shader->type)
    {
        case ShaderType_Null: break;
        case ShaderType_Count: break;
        case ShaderType_Vertex:
        {
            r.context->VSSetShader(shader->vs, nullptr, 0);
            break;
        }
        case ShaderType_Pixel:
        {
            r.context->PSSetShader(shader->ps, nullptr, 0);
            break;
        }
    }
}

void R_ShaderFree(R_Shader* shader)
{
    switch(shader->type)
    {
        case ShaderType_Null: break;
        case ShaderType_Count: break;
        case ShaderType_Vertex:
        {
            SafeRelease(shader->vs);
            break;
        }
        case ShaderType_Pixel:
        {
            SafeRelease(shader->ps);
            break;
        }
    }
}

// Rasterizer state
R_Rasterizer R_RasterizerAlloc(R_RasterizerDesc desc)
{
    auto& r = renderer;
    
    R_Rasterizer res = {};
    
    auto d3d11Desc = D3D11_ConvertRasterizerDesc(desc);
    r.device->CreateRasterizerState(&d3d11Desc, &res.handle);
    return res;
}

void R_RasterizerBind(R_Rasterizer* rasterizer)
{
    auto& r = renderer;
    r.context->RSSetState(rasterizer->handle);
}

void R_RasterizerFree(R_Rasterizer* rasterizer)
{
    SafeRelease(rasterizer->handle);
}

// Depth state
R_DepthState R_DepthStateAlloc(R_DepthDesc desc)
{
    auto& r = renderer;
    
    R_DepthState res = {};
    auto d3d11DepthDesc = D3D11_ConvertDepthStateDesc(desc);
    r.device->CreateDepthStencilState(&d3d11DepthDesc, &res.handle);
    return res;
}

void R_DepthStateBind(R_DepthState* depth)
{
    renderer.context->OMSetDepthStencilState(depth->handle, 0);
}

void R_DepthStateFree(R_DepthState* depth)
{
    SafeRelease(depth->handle);
}

// Textures
R_Texture2D R_Texture2DAlloc(R_TextureFormat format, u32 width, u32 height, void* initData, R_TextureUsage usage, R_TextureMutability mutability)
{
    R_Texture2D res = {};
    res.width = width;
    res.height = height;
    res.formatSimple = format;
    res.format = D3D11_ConvertTextureFormat(format);
    
    D3D11_TEXTURE2D_DESC desc = {0};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = res.format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_ConvertTextureMutability(mutability);
	desc.BindFlags = D3D11_GetTexBindFlags(usage);
	desc.CPUAccessFlags = D3D11_GetTextureCPUAccess(mutability);
	desc.MiscFlags = 0;
    
    D3D11_FormatGetPixelSize(format);
    
    return res;
}

void R_Texture2DBind(R_Texture2D* t, u32 slot, R_ShaderType type)
{
    auto& r = renderer;
    
    switch(type)
    {
        case ShaderType_Null:  break;
        case ShaderType_Count: break;
        case ShaderType_Vertex: r.context->VSSetShaderResources(slot, 1, &t->resView); break;
        case ShaderType_Pixel:  r.context->PSSetShaderResources(slot, 1, &t->resView); break;
    }
}

void R_Texture2DFree(R_Texture2D* t)
{
    SafeRelease(t->handle);
    SafeRelease(t->resView);
}

// Samplers
R_Sampler R_SamplerAlloc(R_SamplerFilter min, R_SamplerFilter mag, R_SamplerWrap wrapU, R_SamplerWrap wrapV)
{
    auto& r = renderer;
    
    R_Sampler res = {};
    
    D3D11_SAMPLER_DESC desc = {};
    desc.Filter = (D3D11_FILTER)D3D11_ConvertSamplerFilter(min, mag);
    desc.AddressU = (D3D11_TEXTURE_ADDRESS_MODE)D3D11_ConvertSamplerWrap(wrapU);
    desc.AddressV = (D3D11_TEXTURE_ADDRESS_MODE)D3D11_ConvertSamplerWrap(wrapV);
    desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    
    r.device->CreateSamplerState(&desc, &res.handle);
    return res;
}

void R_SamplerBind(R_Sampler* s, u32 slot, R_ShaderType type)
{
    auto& r = renderer;
    
    switch(type)
    {
        case ShaderType_Null:  break;
        case ShaderType_Count: break;
        case ShaderType_Vertex: r.context->VSSetSamplers(slot, 1, &s->handle); break;
        case ShaderType_Pixel:  r.context->PSSetSamplers(slot, 1, &s->handle); break;
    }
}

void R_SamplerFree(R_Sampler* sampler)
{
    SafeRelease(sampler->handle);
}

// Framebuffers
R_Framebuffer R_FramebufferAlloc(u32 width, u32 height, R_Texture2D* colorAttachments, u32 colorAttachmentsCount, R_Texture2D depthStencilAttachment)
{
    assert(colorAttachmentsCount > 0);
    
    auto& r = renderer;
    
    if(width < 1)  width = 1;
    if(height < 1) height = 1;
    
    R_Framebuffer res = {};
    res.width = width;
    res.height = height;
    if(colorAttachmentsCount > 0)
    {
        res.colorFormat = colorAttachments[0].format;
        res.colorFormatSimple = colorAttachments[0].formatSimple;
    }
    
    Resize(&res.colorTextures, colorAttachmentsCount);
    Resize(&res.rtv, colorAttachmentsCount);
    for(u32 i = 0; i < colorAttachmentsCount; ++i)
    {
        assert(res.colorFormat == colorAttachments[i].format);
        
        res.colorTextures[i] = colorAttachments[i].handle;
        colorAttachments[i].handle->AddRef();
        
        // Create render target view from texture
        HRESULT hr = r.device->CreateRenderTargetView(res.colorTextures[i], nullptr, &res.rtv[i]);
        assert(SUCCEEDED(hr));
    }
    
    // Depth stencil target view
    TODO;
    
    return res;
}

const R_Framebuffer* R_GetScreen()
{
    return &renderer.screen;
}

void R_FramebufferBind(const R_Framebuffer* f)
{
    renderer.context->OMSetRenderTargets(f->rtv.len, f->rtv.ptr, f->dsv);
}

void R_FramebufferResize(R_Framebuffer* f, u32 newWidth, u32 newHeight)
{
    // Need to recreate all textures and associated views.
}

void R_FramebufferClear(const R_Framebuffer* f, R_BufferMask mask)
{
    auto& r = renderer;
    
    u8 zeros[32] = {};
    if(mask & BufferMask_Color)
    {
        for(int i = 0; i < f->rtv.len; ++i)
            r.context->ClearRenderTargetView(f->rtv[i], (float*)zeros);
    }
    
    u32 clearFlag = 0;
    if(mask & BufferMask_Depth)   clearFlag |= D3D11_CLEAR_DEPTH;
    if(mask & BufferMask_Stencil) clearFlag |= D3D11_CLEAR_STENCIL;
    
    if(clearFlag != 0)
        r.context->ClearDepthStencilView(f->dsv, clearFlag, 1.0f, 0);
}

void R_FramebufferFillColorFloat(const R_Framebuffer* f, u32 slot, f64 r, f64 g, f64 b, f64 a)
{
    assert((s32)slot < f->rtv.len);
    float array[4] = { (float)r, (float)g, (float)b, (float)a };
    renderer.context->ClearRenderTargetView(f->rtv[slot], array);
}

void R_FramebufferFillColorInt(const R_Framebuffer* f, u32 slot, s32 r, s32 g, s32 b, s32 a)
{
    assert((s32)slot < f->rtv.len);
    
    // The microsoft docs recommend drawing a full screen quad
    // for clearing integer textures:
    // https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-clearrendertargetview#remarks
    
    TODO;
}

IVec4 R_FramebufferReadColor(const R_Framebuffer* f, u32 slot, u32 x, u32 y)
{
    assert(!R_IsInteger(f->colorFormatSimple));
    assert((s32)slot < f->rtv.len);
    assert((s32)slot < f->colorTextures.len);
    assert(f->rtv.len == f->colorTextures.len);
    
    auto& r = renderer;
    HRESULT hr;
    
    // We need to create a staging texture, intended for CPU access
    // TODO: We actually don't need to create a separate texture if the
    // texture was created with CPU read access (i think). Consider writing a separate
    // code path in that case.
    
    D3D11_TEXTURE2D_DESC stagingDesc = {};
    stagingDesc.Width = f->width;
    stagingDesc.Height = f->height;
    stagingDesc.MipLevels = 1;
    stagingDesc.ArraySize = 1;
    stagingDesc.Format = f->colorFormat;
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;  // No binding, only staging
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    
    ID3D11Texture2D* stagingTexture = nullptr;
    defer { SafeRelease(stagingTexture); };
    hr = r.device->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture);
    assert(SUCCEEDED(hr));
    
    // Copy rtv texture data to staging texture
    r.context->CopyResource(stagingTexture, f->colorTextures[slot]);
    
    ID3D11RenderTargetView* rtv = nullptr;
    defer { SafeRelease(rtv); };
    hr = r.device->CreateRenderTargetView(f->colorTextures[slot], nullptr, &rtv);
    assert(SUCCEEDED(hr));
    
    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    hr = r.context->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
    defer { r.context->Unmap(stagingTexture, 0); };
    if(SUCCEEDED(hr))
    {
        // Convert x and y coordinates from (0, 0) at bottom left to
        // (0, 0) at top left.
        y = f->height - 1 - y;
        
        // Access pixel data on the CPU
        u32 pixelSize = D3D11_FormatGetPixelSize(f->colorFormatSimple);
        u8* pixelData = (u8*)mappedResource.pData;
        u8* pixelAddr = (pixelData + (y * mappedResource.RowPitch) + (x * pixelSize));
        u32 numChannels = R_NumChannels(f->colorFormatSimple);
        
        IVec4 res = {};
        if(numChannels > 0) res.x = pixelAddr[0];
        if(numChannels > 1) res.y = pixelAddr[1];
        if(numChannels > 2) res.z = pixelAddr[2];
        if(numChannels > 3) res.w = pixelAddr[3];
        return res;
    }
    else
        return {0, 0, 0, 0};
}

void R_FramebufferFree(R_Framebuffer* f)
{
    for(int i = 0; i < f->rtv.len; ++i)
        SafeRelease(f->rtv[0]);
    
    Free(&f->rtv);
    
    SafeRelease(f->dsv);
    
    for(int i = 0; i < f->colorTextures.len; ++i)
        SafeRelease(f->colorTextures[i]);
}

// Vertex layouts
R_VertLayout R_VertLayoutAlloc(R_VertAttrib* attributes, u32 count)
{
    auto& r = renderer;
    
    ScratchArena scratch;
    auto descs = ArenaZAllocArray(D3D11_INPUT_ELEMENT_DESC, count, scratch);
    
    for(u32 i = 0; i < count; ++i)
        descs[i] = D3D11_ConvertVertAttrib(attributes[i]);
    
    // NOTE: For some reason, D3D11 requires you to pass the bytecode of a shader with
    // a matching vertex layout to the one you pass, so that it can check that they're
    // actually the same (?????), even though a vertex layout can be used with multiple
    // different shaders. For this reason, I generate a dummy shader every time a new
    // vertex layout is created, to compile it and use it to create the input layout.
    
    String vertShaderSrc = D3D11_BuildDummyShaderForInputLayout({attributes, count}, scratch);
    ID3DBlob* bytecode = nullptr;
    ID3DBlob* errors   = nullptr;
    HRESULT hr = D3DCompile(vertShaderSrc.ptr, vertShaderSrc.len,
                            "dummy_vertex_shader",
                            nullptr,   // Defines
                            nullptr,   // Includes
                            "main",    // Entrypoint
                            "vs_5_0",  // Target (shader model)
                            0,         // Flags1
                            0,         // Flags2
                            &bytecode,
                            &errors);
    defer { SafeRelease(bytecode); SafeRelease(errors); };
    
    if(errors)
    {
        Log("%s", (char*)errors->GetBufferPointer());
        assert(false);
    }
    
    R_VertLayout res = {};
    hr = r.device->CreateInputLayout(descs, count, bytecode->GetBufferPointer(), bytecode->GetBufferSize(), &res.handle);
    assert(SUCCEEDED(hr));
    return res;
}

void R_VertLayoutBind(R_VertLayout* layout)
{
    renderer.context->IASetInputLayout(layout->handle);
}

void R_VertLayoutFree(R_VertLayout* layout)
{
    
}

// Rendering operations
void R_SetViewport(s32 x, s32 y, int width, int height)
{
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = (float)x;
    viewport.TopLeftY = (float)y;
    viewport.Width    = (float)width;
    viewport.Height   = (float)height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    
    renderer.context->RSSetViewports(1, &viewport);
}

void R_Draw(R_Buffer* verts, R_Buffer* indices, u64 start, u64 count)
{
    auto& r = renderer;
    
    // Set buffers
    /*r.context->IASetIndexBuffer(indices->handle, DXGI_FORMAT_R32_UINT, 0);
    r.context->IASetVertexBuffers(verts->handle, 0, 1, verts->handle, &verts->stride);
    r.context->Draw(count, start);
*/
}

void R_Draw(R_Buffer* verts, u64 start, u64 count)
{
    auto& r = renderer;
    
    UINT offset = 0;
    r.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    r.context->IASetVertexBuffers(0, 1, &verts->handle, &verts->stride, &offset);
    
    if(count == 0) count = verts->size / verts->stride;
    
    r.context->Draw((UINT)count, (UINT)start);
}

// Backend state

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
        defer { SafeRelease(swapchain1); };
        
        // Get DXGI device from D3D11 device
        IDXGIDevice* dxgiDevice = nullptr;
        defer { SafeRelease(dxgiDevice); };
        HRESULT res = r.device->QueryInterface(IID_IDXGIDevice, (void**)&dxgiDevice);
        assert(SUCCEEDED(res));
        
        // Get DXGI adapter from DXGI device
        IDXGIAdapter* dxgiAdapter = nullptr;
        defer { SafeRelease(dxgiAdapter); };
        res = dxgiDevice->GetAdapter(&dxgiAdapter);
        assert(SUCCEEDED(res));
        
        // Get DXGI factory from DXGI adapter
        IDXGIFactory2* factory = nullptr;
        defer { SafeRelease(factory); };
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
        
        HWND window = (HWND)Win32_GetWindowHandle();
        res = factory->CreateSwapChainForHwnd((IUnknown*)r.device, window, &swapchainDesc, nullptr, nullptr, &swapchain1);
        assert(SUCCEEDED(res));
        
        swapchain1->QueryInterface(IID_IDXGISwapChain2, (void**)&r.swapchain);
        r.swapchainWaitableObject = r.swapchain->GetFrameLatencyWaitableObject();
        assert(r.swapchainWaitableObject);
        
        // Disable Alt+Enter changing monitor resolution to match window size
        factory->MakeWindowAssociation(win32.window, DXGI_MWA_NO_ALT_ENTER);
    }
    
    // Create screen RTV and DSV
    {
        ID3D11Texture2D* backBuffer = nullptr;
        res = r.swapchain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backBuffer);
        assert(SUCCEEDED(res));
        defer { SafeRelease(backBuffer); };
        
        ID3D11RenderTargetView* screenRtv = nullptr;
        res = r.device->CreateRenderTargetView(backBuffer, nullptr, &screenRtv);
        assert(SUCCEEDED(res));
        Append(&r.screen.rtv, screenRtv);
        
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
        defer { SafeRelease(depthStencilBuffer); };
        
        hr = r.device->CreateDepthStencilView(depthStencilBuffer, nullptr, &r.screen.dsv);
        assert(SUCCEEDED(hr));
    }
}

void R_WaitLastFrame()
{
    auto& r = renderer;
    
    // This should stall the CPU until all GPU commands have finished
    DWORD waitRes = WaitForSingleObjectEx(r.swapchainWaitableObject, 10000, false);
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

void R_UpdateSwapchainSize()
{
#if 0
    auto& r = renderer;
    
    static s32 prevW = 0;
    static s32 prevH = 0;
    static bool init = true;
    if(init)
    {
        init = false;
        OS_GetClientAreaSize(&prevW, &prevH);
    }
    
    s32 w, h;
    OS_GetClientAreaSize(&w, &h);
    
    if(prevW != w || prevH != h)
    {
        prevW = w;
        prevH = h;
        
        R_FramebufferFree(&r.screen);
        renderer.swapchain->ResizeBuffers(2, w, h,
                                          DXGI_FORMAT_UNKNOWN,  // Preserves the previous value
                                          DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
        
        ID3D11Texture2D* backBuffer = nullptr;
        HRESULT res = r.swapchain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backBuffer);
        assert(SUCCEEDED(res));
        defer { SafeRelease(backBuffer); };
        
        ID3D11RenderTargetView* screenRtv = nullptr;
        res = r.device->CreateRenderTargetView(backBuffer, nullptr, &screenRtv);
        assert(SUCCEEDED(res));
        Append(&r.screen.rtv, screenRtv);
        
        ID3D11Texture2D* depthStencilBuffer;
        HRESULT hr = r.device->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencilBuffer);
        assert(SUCCEEDED(hr));
        defer { SafeRelease(depthStencilBuffer); };
        
        hr = r.device->CreateDepthStencilView(depthStencilBuffer, nullptr, &r.screen.dsv);
        assert(SUCCEEDED(hr));
    }
#endif
}

void R_Cleanup()
{
    auto& r = renderer;
    
    SafeRelease(r.device);
    SafeRelease(r.context);
    SafeRelease(r.swapchain);
    R_FramebufferFree(&r.screen);
    
#ifdef Development
    IDXGIDebug1* debug = nullptr;
    if(SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
    {
        debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
        debug->Release();
    }
#endif
}

// Dear ImGui
void R_ImGuiInit()
{
    ImGui_ImplDX11_Init(renderer.device, renderer.context);
}

void R_ImGuiShutdown()
{
    ImGui_ImplDX11_Shutdown();
}

void R_ImGuiNewFrame()
{
    ImGui_ImplDX11_NewFrame();
}

void R_ImGuiDrawFrame()
{
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}


// D3D11 Utility functions
static u32 D3D11_FormatGetPixelSize(R_TextureFormat format)
{
    switch(format)
    {
        case TextureFormat_Invalid:      return 0;
        case TextureFormat_Count:        return 0;
        case TextureFormat_R32Int:       return 4;
        case TextureFormat_R:            return 1;
        case TextureFormat_RG:           return 2;
        case TextureFormat_RGBA:         return 4;
        case TextureFormat_DepthStencil: return 4;
    }
    
    return 0;
}

static u32 D3D11_GetBufferBindFlags(R_BufferFlags flags)
{
    u32 res = 0;
    if(flags & BufferFlag_Vertex)         res |= D3D11_BIND_VERTEX_BUFFER;
    if(flags & BufferFlag_Index)          res |= D3D11_BIND_INDEX_BUFFER;
    if(flags & BufferFlag_ConstantBuffer) res |= D3D11_BIND_CONSTANT_BUFFER;
    
    return res;
}

static u32 D3D11_GetTexBindFlags(R_TextureUsage usage)
{
    u32 res = 0;
    if(usage & TextureUsage_ShaderResource) res |= D3D11_BIND_SHADER_RESOURCE;
    if(usage & TextureUsage_Drawable)       res |= D3D11_BIND_RENDER_TARGET;
    
    return res;
}

static DXGI_FORMAT D3D11_ConvertTextureFormat(R_TextureFormat format)
{
    switch(format)
    {
        case TextureFormat_Invalid: return DXGI_FORMAT_UNKNOWN;
        case TextureFormat_Count:   return DXGI_FORMAT_UNKNOWN;
        case TextureFormat_R32Int:  return DXGI_FORMAT_R32_SINT;
        case TextureFormat_R:       return DXGI_FORMAT_R8_UNORM;
        case TextureFormat_RG:      return DXGI_FORMAT_R8G8_UNORM;
        case TextureFormat_RGBA:    return DXGI_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat_DepthStencil: return DXGI_FORMAT_D24_UNORM_S8_UINT;
    }
    
    return DXGI_FORMAT_UNKNOWN;
}

static D3D11_USAGE D3D11_ConvertTextureMutability(R_TextureMutability mut)
{
    switch(mut)
    {
        case TextureMutability_Count:     return D3D11_USAGE_IMMUTABLE;
        case TextureMutability_Immutable: return D3D11_USAGE_IMMUTABLE;
        case TextureMutability_Mutable:   return D3D11_USAGE_DEFAULT;
        case TextureMutability_Dynamic:   return D3D11_USAGE_DYNAMIC;
    }
    
    return D3D11_USAGE_IMMUTABLE;
}

static u32 D3D11_GetTextureCPUAccess(R_TextureMutability mut)
{
    switch(mut)
    {
        case TextureMutability_Count:     return 0;
        case TextureMutability_Immutable: return 0;
        case TextureMutability_Mutable:   return 0;
        case TextureMutability_Dynamic:   return D3D11_CPU_ACCESS_WRITE;
    }
    
    return D3D11_USAGE_IMMUTABLE;
}

static u32 D3D11_ConvertSamplerFilter(R_SamplerFilter min, R_SamplerFilter mag)
{
    switch(min)
    {
        case SamplerFilter_Count: return 0;
        
        case SamplerFilter_Nearest:
        {
            switch(mag)
            {
                case SamplerFilter_Count: return 0;
                case SamplerFilter_Nearest: return D3D11_FILTER_MIN_MAG_MIP_POINT;
                case SamplerFilter_Linear: return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
                case SamplerFilter_LinearMipmapLinear: return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
                case SamplerFilter_LinearMipmapNearest: return D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
                case SamplerFilter_NearestMipmapLinear: return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
                case SamplerFilter_NearestMipmapNearest: return D3D11_FILTER_MIN_MAG_MIP_POINT;
            }
        }
        break;
        
        case SamplerFilter_Linear:
        {
            switch(mag)
            {
                case SamplerFilter_Count: return 0;
                case SamplerFilter_Nearest: return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
                case SamplerFilter_Linear: return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
                case SamplerFilter_LinearMipmapLinear: return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
                case SamplerFilter_LinearMipmapNearest: return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
                case SamplerFilter_NearestMipmapLinear: return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
                case SamplerFilter_NearestMipmapNearest: return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            }
        }
        break;
        
        case SamplerFilter_LinearMipmapLinear:
        {
            switch(mag)
            {
                case SamplerFilter_Count: return 0;
                case SamplerFilter_Nearest: return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
                case SamplerFilter_Linear: return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
                case SamplerFilter_LinearMipmapLinear: return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
                case SamplerFilter_LinearMipmapNearest: return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
                case SamplerFilter_NearestMipmapLinear: return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
                case SamplerFilter_NearestMipmapNearest: return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            }
        }
        break;
        
        case SamplerFilter_LinearMipmapNearest:
        {
            switch(mag)
            {
                case SamplerFilter_Count: return 0;
                case SamplerFilter_Nearest: return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
                case SamplerFilter_Linear: return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
                case SamplerFilter_LinearMipmapLinear: return D3D11_FILTER_MIN_MAG_MIP_POINT;
                case SamplerFilter_LinearMipmapNearest: return D3D11_FILTER_MIN_MAG_MIP_POINT;
                case SamplerFilter_NearestMipmapLinear: return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
                case SamplerFilter_NearestMipmapNearest: return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
            }
        }
        break;
        
        case SamplerFilter_NearestMipmapLinear:
        {
            switch(mag)
            {
                case SamplerFilter_Count: return 0;
                case SamplerFilter_Nearest: return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
                case SamplerFilter_Linear: return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
                case SamplerFilter_LinearMipmapLinear: return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
                case SamplerFilter_LinearMipmapNearest: return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
                case SamplerFilter_NearestMipmapLinear: return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
                case SamplerFilter_NearestMipmapNearest: return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
            }
        }
        break;
        
        case SamplerFilter_NearestMipmapNearest:
        {
            switch(mag)
            {
                case SamplerFilter_Count: return 0;
                case SamplerFilter_Nearest: return D3D11_FILTER_MIN_MAG_MIP_POINT;
                case SamplerFilter_Linear: return D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
                case SamplerFilter_LinearMipmapLinear: return D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
                case SamplerFilter_LinearMipmapNearest: return D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
                case SamplerFilter_NearestMipmapLinear: return D3D11_FILTER_MIN_MAG_MIP_POINT;
                case SamplerFilter_NearestMipmapNearest: return D3D11_FILTER_MIN_MAG_MIP_POINT;
            }
        }
        break;
    }
    return 0;
}

static u32 D3D11_ConvertSamplerWrap(R_SamplerWrap wrap)
{
    switch(wrap)
    {
        case SamplerWrap_Count: return D3D11_TEXTURE_ADDRESS_CLAMP;
        case SamplerWrap_ClampToEdge: return D3D11_TEXTURE_ADDRESS_CLAMP;
        case SamplerWrap_ClampToBorder: return D3D11_TEXTURE_ADDRESS_BORDER;
        case SamplerWrap_Repeat: return D3D11_TEXTURE_ADDRESS_WRAP;
        case SamplerWrap_MirroredRepeat: return D3D11_TEXTURE_ADDRESS_MIRROR;
        case SamplerWrap_MirrorClampToEdge: return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
    }
    
    return D3D11_TEXTURE_ADDRESS_CLAMP;
}

static D3D11_INPUT_ELEMENT_DESC D3D11_ConvertVertAttrib(R_VertAttrib attrib)
{
    // TODO: Look into instancing
    
    switch(attrib.type)
    {
        case VertAttrib_Pos:
        return { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, attrib.bufferSlot, attrib.offset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
        case VertAttrib_Normal:
        return { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, attrib.bufferSlot, attrib.offset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
        case VertAttrib_TexCoord:
        return { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, attrib.bufferSlot, attrib.offset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
        case VertAttrib_Tangent:
        return { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, attrib.bufferSlot, attrib.offset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
        case VertAttrib_Bitangent:
        return { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, attrib.bufferSlot, attrib.offset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
        case VertAttrib_Color:
        return { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, attrib.bufferSlot, attrib.offset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    }
    
    return {};
}

static String D3D11_BuildDummyShaderForInputLayout(Slice<R_VertAttrib> attribs, Arena* dst)
{
    StringBuilder builder = {};
    UseArena(&builder, dst);
    
    Append(&builder, "struct Vertex\n");
    Append(&builder, "{\n");
    
    for(int i = 0; i < attribs.len; ++i)
    {
        switch(attribs[i].type)
        {
            case VertAttrib_Pos:       Append(&builder, "float3 pos : POSITION;\n");        break;
            case VertAttrib_Normal:    Append(&builder, "float3 normal : NORMAL;\n");       break;
            case VertAttrib_TexCoord:  Append(&builder, "float2 texCoord : TEXCOORD;\n");   break;
            case VertAttrib_Tangent:   Append(&builder, "float3 tangent : TANGENT;\n");     break;
            case VertAttrib_Bitangent: Append(&builder, "float3 bitangent : BITANGENT;\n"); break;
            case VertAttrib_Color:     Append(&builder, "float3 color : COLOR;\n");         break;
        }
    }
    
    Append(&builder, "};\n");
    Append(&builder, "\n");
    
    Append(&builder, "float4 main(Vertex vert) : SV_POSITION\n");
    Append(&builder, "{\n");
    Append(&builder, "    return (float4)0.0f;\n");
    Append(&builder, "}\n");
    
    return ToString(&builder);
}

static D3D11_RASTERIZER_DESC D3D11_ConvertRasterizerDesc(R_RasterizerDesc desc)
{
    D3D11_RASTERIZER_DESC res = {};
    switch(desc.cullMode)
    {
        case CullMode_None:  res.CullMode = D3D11_CULL_NONE;  break;
        case CullMode_Front: res.CullMode = D3D11_CULL_FRONT; break;
        case CullMode_Back:  res.CullMode = D3D11_CULL_BACK;  break;
    }
    
    res.FillMode = D3D11_FILL_SOLID;
    res.FrontCounterClockwise = desc.frontCounterClockwise;
    res.DepthBias = desc.depthBias;
    res.DepthBiasClamp = desc.depthBiasClamp;
    res.ScissorEnable = desc.scissorEnable;
    res.DepthClipEnable = desc.depthClipEnable;
    return res;
}

static D3D11_DEPTH_STENCIL_DESC D3D11_ConvertDepthStateDesc(R_DepthDesc desc)
{
    D3D11_DEPTH_STENCIL_DESC res = {};
    res.DepthEnable = desc.depthEnable;
    
    switch(desc.depthWriteMask)
    {
        case DepthWriteMask_Zero: res.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; break;
        case DepthWriteMask_All: res.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; break;
    }
    
    switch(desc.depthFunc)
    {
        case DepthFunc_Never: res.DepthFunc = D3D11_COMPARISON_NEVER; break;
        case DepthFunc_Less: res.DepthFunc = D3D11_COMPARISON_LESS; break;
        case DepthFunc_Equal: res.DepthFunc = D3D11_COMPARISON_EQUAL; break;
        case DepthFunc_LessEqual: res.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; break;
        case DepthFunc_Greater: res.DepthFunc = D3D11_COMPARISON_GREATER; break;
        case DepthFunc_NotEqual: res.DepthFunc = D3D11_COMPARISON_NOT_EQUAL; break;
        case DepthFunc_GreaterEqual: res.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL; break;
        case DepthFunc_Always: res.DepthFunc = D3D11_COMPARISON_ALWAYS; break;
    }
    
    res.StencilEnable = false;
    return res;
}

// Old code, could still be useful though.
#if 0
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

#endif

// Undefine backend specific macros
#undef SafeRelease
