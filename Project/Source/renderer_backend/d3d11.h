
#pragma once

#include "base.h"
#include <d3d11.h>

#pragma warning(push)  // These includes have tons of warnings so we're disabling them
#pragma warning(disable : 4062)
#pragma warning(disable : 4061)
#include <d3d11.h>
#include <dxgi1_3.h>
#include <dxgidebug.h>
#include <d3d11sdklayers.h>
#include <d3dcompiler.h>
#pragma warning(pop)

struct R_Buffer
{
    R_BufferFlags flags;
    ID3D11Buffer* handle;
    u32 stride;
    u64 size;
};

struct R_VertLayout
{
    ID3D11InputLayout* handle;
    u32 stride;
};

enum ShaderType;
struct R_Shader
{
    ShaderType type;
    union
    {
        ID3D11VertexShader* vs;
        ID3D11PixelShader* ps;
    };
};

struct R_Rasterizer
{
    ID3D11RasterizerState* handle;
};

struct R_DepthState
{
    ID3D11DepthStencilState* handle;
};

struct R_Texture2D
{
    u32 width, height;
    DXGI_FORMAT format;
    R_TextureFormat formatSimple;
    D3D11_USAGE usage;
    
    ID3D11Texture2D* handle;
    ID3D11ShaderResourceView* resView;  // Can be nullptr
};

struct R_Sampler
{
    ID3D11SamplerState* handle;
};

struct R_Framebuffer
{
    u32 width, height;
    DXGI_FORMAT colorFormat;
    R_TextureFormat colorFormatSimple;
    
    Array<ID3D11RenderTargetView*> rtv;
    ID3D11DepthStencilView* dsv;
    
    Array<ID3D11Texture2D*> colorTextures;
    ID3D11Texture2D* depthStencilTexture;
};

struct Renderer
{
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    ID3D11Texture2D* backbuffer;
    IDXGISwapChain2* swapchain;
    // This lets us wait until the last frame has been presented,
    // which is good because it lets us reduce latency
    HANDLE swapchainWaitableObject;
    
    R_Framebuffer screen;
};
