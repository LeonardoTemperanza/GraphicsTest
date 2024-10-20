
#pragma once

#include "base.h"
#include <d3d11.h>

struct R_Mesh
{
    ID3D11Buffer* vertBuf;
    ID3D11Buffer* indexBuf;
    u32 numIndices;
};

struct R_Texture
{
    R_TextureKind kind;
    
    ID3D11Texture2D* tex;
    ID3D11ShaderResourceView* view;
};

struct R_Sampler
{
    ID3D11SamplerState* sampler;
};

struct R_Framebuffer
{
    ID3D11Texture2D* color;         // May be null
    ID3D11Texture2D* depthStencil;  // May be null
    
    ID3D11RenderTargetView* rtv;  // May be null
    ID3D11DepthStencilView* dsv;  // May be null
    
    ID3D11ShaderResourceView* colorShaderInput;  // May be null
    
    R_TextureFormat format;
    
    int width, height;
};

struct R_Shader
{
    ShaderKind kind;
    Array<ShaderValType> codeConstantTypes;
    Array<ShaderValType> materialConstantTypes;
    u32 materialTexturesCount;
    u32 codeTexturesCount;
    u32 codeSamplersCount;
    
    ID3D11Buffer* codeConstants;
    ID3D11Buffer* materialConstants;
    
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

struct IDXGISwapChain2;
struct Renderer
{
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    IDXGISwapChain2* swapchain;
    // This lets us wait until the last frame has been presented,
    // which is good because it lets us reduce latency
    HANDLE swapchainWaitableObject;
    
    // States
    D3D11_BLEND_DESC blendDesc;
    ID3D11BlendState* blendState;
    ID3D11RasterizerState* rasterizerState;
    ID3D11DepthStencilState* depthState;
    
    // Swapchain resources
    ID3D11Texture2D* backbufferColor;
    ID3D11Texture2D* backbufferDepthStencil;
    
    // Swapchain views
    ID3D11RenderTargetView* rtv;
    ID3D11DepthStencilView* dsv;
    
    // Opengl-like bindings
    ID3D11RenderTargetView* boundRtv;
    ID3D11DepthStencilView* boundDsv;
    
    // Buffers used for immediate mode rendering style
    ID3D11Buffer* quadVerts;
    
    // Commonly used input layouts
    ID3D11InputLayout* staticMeshInputLayout;
    
    // Commonly used samplers
    ID3D11SamplerState* samplers[R_SamplerCount];
    
    // Constant buffers
    ID3D11Buffer* perSceneBuf;
    ID3D11Buffer* perFrameBuf;
    ID3D11Buffer* perObjBuf;
};
