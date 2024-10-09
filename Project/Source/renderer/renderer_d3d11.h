
#pragma once

#include "base.h"

struct ID3D11Buffer;
struct R_Mesh
{
    ID3D11Buffer* vertBuf;
    ID3D11Buffer* indexBuf;
    u32 numIndices;
};

struct ID3D11ShaderResourceView;
struct ID3D11Texture2D;
struct R_Texture
{
    R_TextureKind kind;
    
    ID3D11Texture2D* tex;
    ID3D11ShaderResourceView* view;
};

struct ID3D11SamplerState;
struct R_Sampler
{
    ID3D11SamplerState* sampler;
};

struct R_Framebuffer
{
    u32 a;
    // Textures
};

struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11ComputeShader;
struct R_Shader
{
    ShaderKind kind;
    
    union
    {
        ID3D11VertexShader* vertShader;
        ID3D11PixelShader* pixelShader;
        ID3D11ComputeShader* computeShader;
    };
};

struct R_Pipeline
{
    ID3D11VertexShader* vertShader;
    ID3D11PixelShader* pixelShader;
};

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain2;
struct ID3D11RasterizerState;
struct ID3D11RenderTargetView;
struct ID3D11InputLayout;

struct Renderer
{
    ID3D11Device* device;
    ID3D11DeviceContext* deviceContext;
    IDXGISwapChain2* swapchain;
    // This lets us wait until the last frame has been presented,
    // which is good because it lets us reduce latency
    HANDLE swapchainWaitableObject;
    
    ID3D11RenderTargetView* rtv;
    
    // Buffers used for immediate mode rendering style
    ID3D11Buffer* quadVerts;
    
    // Commonly used input layouts
    ID3D11InputLayout* staticMeshInputLayout;
    
    // Commonly used samplers
    ID3D11SamplerState* samplers[R_SamplerCount];
};
