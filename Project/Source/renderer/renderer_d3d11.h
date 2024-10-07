
#pragma once

#include "base.h"

struct R_Mesh
{
    u32 test;
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

struct R_Shader
{
    u32 a;
};

struct R_Pipeline
{
    u32 a;
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
    HANDLE swapchainWaitableObject;  // This lets us wait until the last frame has been presented
    
    ID3D11RenderTargetView* rtv;
};