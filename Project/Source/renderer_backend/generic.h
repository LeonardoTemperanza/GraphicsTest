
#pragma once

#include "base.h"

// NOTE: This is meant to be a thin abstraction over the graphics apis. No application logic
// should be handled here, as that's meant to be a job for the renderer frontend.

// Resources

typedef u32 R_BufferFlags;
enum
{
    BufferFlag_Dynamic        = 1 << 0,
    BufferFlag_Vertex         = 1 << 1,
    BufferFlag_Index          = 1 << 2,
    BufferFlag_ConstantBuffer = 1 << 3
};

enum R_VertAttribType
{
    VertAttrib_Pos = 0,
    VertAttrib_Normal,
    VertAttrib_TexCoord,
    VertAttrib_Tangent,
    VertAttrib_Bitangent,
    VertAttrib_ColorRGB,
    VertAttrib_ColorScale,
};

struct R_VertAttrib
{
    R_VertAttribType type;
    u32 typeSlot;  // One could use the same type more than once
    u32 bufferSlot;
    u32 offset;
};

enum R_BlendMode
{
    BlendMode_None = 0,
    BlendMode_Alpha,
    
    BlendMode_Count
};

enum R_SamplerWrap
{
    SamplerWrap_ClampToEdge,
    SamplerWrap_ClampToBorder,
    SamplerWrap_Repeat,
    SamplerWrap_MirroredRepeat,
    SamplerWrap_MirrorClampToEdge,
    
    SamplerWrap_Count
};

enum R_SamplerFilter
{
    SamplerFilter_Nearest,
    SamplerFilter_Linear,
    SamplerFilter_LinearMipmapLinear,
    SamplerFilter_LinearMipmapNearest,
    SamplerFilter_NearestMipmapLinear,
    SamplerFilter_NearestMipmapNearest,
    
    SamplerFilter_Count
};

enum R_TextureFormat
{
    TextureFormat_Invalid = 0,
    TextureFormat_R32Int,
    TextureFormat_R,
    TextureFormat_RG,
    TextureFormat_RGBA,
    TextureFormat_RGBA_SRGB,
    TextureFormat_RGBA_HDR,
    TextureFormat_DepthStencil,
    
    TextureFormat_Count
};

enum R_TextureMutability
{
    TextureMutability_Immutable = 0,  // Can only be read from the GPU
    TextureMutability_Mutable,        // Can only be read and written from the GPU
    TextureMutability_Dynamic,        // Can also be written from the CPU. Expected for resources that change CPU side every frame
    
    TextureMutability_Count
};

typedef u32 R_TextureUsage;
enum
{
    TextureUsage_ShaderResource = 1 << 0,
    TextureUsage_Drawable       = 1 << 1,
};

typedef u32 R_BufferMask;
enum
{
    BufferMask_Color   = 1 << 0,
    BufferMask_Depth   = 1 << 1,
    BufferMask_Stencil = 1 << 2,
};

enum R_CullMode
{
    CullMode_None  = 0,
    CullMode_Front = 1,
    CullMode_Back  = 2,
};

struct R_RasterizerDesc
{
    R_CullMode cullMode = CullMode_Back;
    bool frontCounterClockwise = true;
    s32 depthBias = 0;
    f32 depthBiasClamp = 0.0f;
    bool depthClipEnable = true;
    bool scissorEnable = false;
};

enum R_DepthWriteMask
{
    DepthWriteMask_Zero = 0,
    DepthWriteMask_All,
};

enum R_DepthFunc
{
    DepthFunc_Never = 0,
    DepthFunc_Less,
    DepthFunc_Equal,
    DepthFunc_LessEqual,
    DepthFunc_Greater,
    DepthFunc_NotEqual,
    DepthFunc_GreaterEqual,
    DepthFunc_Always,
};

struct R_DepthDesc
{
    bool depthEnable = true;
    R_DepthWriteMask depthWriteMask = DepthWriteMask_All;
    R_DepthFunc depthFunc = DepthFunc_Less;
    
    // TODO: Missing stencil ops
};

struct R_ShaderInput
{
    String d3d11Bytecode;
    String dxil;
    String vulkanSpirv;
    String glsl;
};

// Enums info
u32 R_NumChannels(R_TextureFormat format);
bool R_IsInteger(R_TextureFormat format);

// Backend specific structures...
struct R_Buffer;
struct R_VertLayout;
struct R_Shader;
struct R_ShaderPack;
struct R_BufferAttribPack;
struct R_Pipeline;
struct R_Texture2D;
struct R_Cubemap;
struct R_Framebuffer;
struct R_Rasterizer;
struct R_DepthState;

// ...which are defined here
#ifdef GFX_OPENGL
#include "renderer_backend/opengl.h"
#elif defined(GFX_D3D11)
#include "renderer_backend/d3d11.h"
#else
#error "Unsupported gfx api."
#endif

// Buffers
R_Buffer R_BufferAlloc(R_BufferFlags flags, u32 stride, u64 size = 0, void* initData = nullptr);
#define R_BufferAllocStruct(flags, structName) R_BufferAlloc(flags, sizeof(structName), sizeof(structName), &structName)
// TODO: Macros for allocating arrays and structs easily
void R_BufferUpdate(R_Buffer* b, u64 offset, u64 size, void* data);
#define R_BufferUpdateStruct(buffer, structVar) R_BufferUpdate(buffer, 0, sizeof(structVar), &structVar)
void R_BufferUniformBind(R_Buffer* b, u32 slot, ShaderType type);
void R_BufferFree(R_Buffer* b);

// Shaders
R_Shader R_ShaderAlloc(R_ShaderInput input, ShaderType type);
void R_ShaderBind(R_Shader* shader);
void R_ShaderFree(R_Shader* shader);

// Rasterizer state
R_Rasterizer R_RasterizerAlloc(R_RasterizerDesc desc);
void R_RasterizerBind(R_Rasterizer* r);
void R_RasterizerFree(R_Rasterizer* r);

// Depth state
R_DepthState R_DepthStateAlloc(R_DepthDesc desc);
void R_DepthStateBind(R_DepthState* depth);
void R_DepthStateFree(R_DepthState* depth);

// Textures
R_Texture2D R_Texture2DAlloc(R_TextureFormat format, u32 width, u32 height, void* initData = nullptr,
                             R_TextureUsage usage = TextureUsage_ShaderResource | TextureUsage_Drawable,
                             R_TextureMutability mutability = TextureMutability_Mutable,
                             bool mips = false,
                             u8 sampleCount = 1);
void R_Texture2DTransfer(R_Texture2D* t, String data);
void R_Texture2DBind(R_Texture2D* t, u32 slot, ShaderType type);
void R_Texture2DFree(R_Texture2D* t);
struct R_CubemapBinary
{
    u32 len;
    void* top, *bottom, *left, *right, *front, *back;
};
void R_CubemapAlloc(R_Cubemap* c, u32 width, u32 height, R_CubemapBinary initData,
                    R_TextureUsage usage = TextureUsage_ShaderResource | TextureUsage_Drawable,
                    R_TextureMutability mutability = TextureMutability_Immutable);
void R_CubemapTransfer(R_Cubemap* c, R_CubemapBinary data);
void R_CubemapBind(R_Cubemap* c, u32 slot, ShaderType type);
void R_CubemapFree(R_Cubemap* c);

// Samplers
R_Sampler R_SamplerAlloc(R_SamplerFilter min = SamplerFilter_LinearMipmapLinear,
                         R_SamplerFilter mag = SamplerFilter_LinearMipmapLinear,
                         R_SamplerWrap wrapU = SamplerWrap_Repeat,
                         R_SamplerWrap wrapV = SamplerWrap_Repeat);
void R_SamplerBind(R_Sampler* s, u32 slot, ShaderType type);
void R_SamplerFree(R_Sampler* s);

// Framebuffers
R_Framebuffer R_FramebufferAlloc(u32 width, u32 height, R_Texture2D* colorAttachments,
                                 u32 colorAttachmentsCount, R_Texture2D depthAttachment);
const R_Framebuffer* R_GetScreen();
void R_FramebufferBind(const R_Framebuffer* f);
void R_FramebufferResize(const R_Framebuffer* f, u32 newWidth, u32 newHeight);
void R_FramebufferClear(const R_Framebuffer* f, R_BufferMask mask);
// Unused channels will naturally be ignored
void R_FramebufferFillColor(const R_Framebuffer* f, u32 slot, f64 r, f64 g, f64 b, f64 a);
// NOTE: This causes a stall on the CPU, as it will wait until the data is transferred,
// so try to call this at the end of the frame if necessary
// (0, 0) is located on the bottom left of the image.
// This function returns 0 for unused channels in the corresponding index
IVec4 R_FramebufferReadColor(const R_Framebuffer* f, u32 slot, s32 x, s32 y);
void R_FramebufferResolve(R_Framebuffer* src, R_Framebuffer* dst);
void R_FramebufferFree(R_Framebuffer* f);

// Vertex layouts
R_VertLayout R_VertLayoutAlloc(R_VertAttrib* attributes, u32 count);
void R_VertLayoutBind(R_VertLayout* layout);
void R_VertLayoutFree(R_VertLayout* layout);

// Rendering operations
void R_SetViewport(s32 x, s32 y, s32 w, s32 h);
void R_Draw(R_Buffer* verts, R_Buffer* indices, u64 start = 0, u64 count = 0);  // Count = 0 means the entire mesh
void R_Draw(R_Buffer* verts, u64 start = 0, u64 count = 0);                     // Count = 0 means the entire mesh
void R_SetAlphaBlending(bool enable);

// Backend state
void R_Init();  // Initializes the graphics api context
void R_WaitLastFrame();
void R_PresentFrame();
// NOTE: Should be called after R_WaitLastFrame and before any rendering commands
void R_UpdateSwapchainSize();
void R_Cleanup();

// Miscellaneous
Mat4 R_ConvertClipSpace(Mat4 mat);

// Dear ImGui
void R_ImGuiInit();
void R_ImGuiShutdown();
void R_ImGuiNewFrame();
void R_ImGuiDrawFrame();
