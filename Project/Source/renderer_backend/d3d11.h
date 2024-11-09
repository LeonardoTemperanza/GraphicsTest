
#pragma once

#include "base.h"

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
    u32 vertStride;
};

struct R_Shader
{
    R_ShaderType type;
    ID3D10Blob* bytecode;
    union
    {
        ID3D11VertexShader* vs;
        ID3D11PixelShader* ps;
    };
};

struct R_ShaderPack
{
    R_Shader vs;
    R_Shader ps;
};

struct R_Pipeline
{
    R_InputAssembly assembly;
    
    R_BlendMode blendMode;
    ID3D11InputLayout* layout;
    ID3D11BlendState* blendState;
    bool layoutChanged;
};

struct R_Sampler
{
    ID3D11SamplerState* handle;
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

struct R_Framebuffer
{
    u32 width, height;
    DXGI_FORMAT colorFormat;
    R_TextureFormat colorFormatSimple;
    
    Array<ID3D11RenderTargetView*> rtv;
    ID3D11DepthStencilView* dsv;  // Could be nullptr
    
    // In the case of the screen framebuffer this can't be accessed
    // so this will actually be empty
    Array<ID3D11Texture2D*> colorTextures;
};

struct Renderer
{
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    IDXGISwapChain2* swapchain;
    // This lets us wait until the last frame has been presented,
    // which is good because it lets us reduce latency
    HANDLE swapchainWaitableObject;
    
    R_Framebuffer screen;
};







#if 0

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

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain2;
struct ID3D11BlendState;
struct ID3D11RasterizerState;
struct ID3D11DepthStencilState;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11InputLayout;

struct Renderer
{
    ID3D11Device* device;
    ID3D11DeviceContext* deviceContext;
    IDXGISwapChain2* swapchain;
    // This lets us wait until the last frame has been presented,
    // which is good because it lets us reduce latency
    HANDLE swapchainWaitableObject;
    
    // States
    ID3D11BlendState* blendState;
    ID3D11RasterizerState* rasterizerState;
    ID3D11DepthStencilState* depthState;
    
    // Swapchain views
    ID3D11RenderTargetView* rtv;
    ID3D11DepthStencilView* dsv;
    
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
#endif