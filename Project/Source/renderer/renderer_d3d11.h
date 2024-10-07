
#pragma once

#include "base.h"

struct ID3D11Buffer;
struct R_Mesh
{
    ID3D11Buffer* vertBuf;
    ID3D11Buffer* indexBuf;
    u32 numIndices;
};

struct R_Texture
{
    u32 test;
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
};
