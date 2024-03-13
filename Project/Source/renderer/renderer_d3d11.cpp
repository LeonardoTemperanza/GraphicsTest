
#include "renderer_d3d11.h"

D3D11_Context Win32_GetD3D11Context();

d3d11_Renderer* d3d11_InitRenderer(Arena* permArena)
{
    // @temp Maybe some of this stuff can be moved to the win32 layer
    
    auto res = (d3d11_Renderer*)malloc(sizeof(d3d11_Renderer));
    memset(res, 0, sizeof(d3d11_Renderer));
    
    D3D11_Context ctx = Win32_GetD3D11Context();
    
    // Framebuffer
    {
        ctx.swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&res->framebuffer);
        
        D3D11_RENDER_TARGET_VIEW_DESC desc = {0};
        desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        
        ID3D11RenderTargetView* view;
        ctx.device->CreateRenderTargetView(res->framebuffer, &desc, &view);
    }
    
    // Depth buffer
    {
        D3D11_TEXTURE2D_DESC desc = {0};
        res->framebuffer->GetDesc(&desc);
        desc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        
        ctx.device->CreateTexture2D(&desc, nullptr, &res->depthBuffer);
        ID3D11DepthStencilView* depthStencilView;
        ctx.device->CreateDepthStencilView(res->depthBuffer, nullptr, &depthStencilView);
    }
    
    return {0};
}

void d3d11_Render(d3d11_Renderer* r, RenderSettings renderSettings)
{
    
}