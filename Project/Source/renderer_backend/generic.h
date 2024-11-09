
#pragma once

#include "base.h"
#include "serialization.h"

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

enum R_ShaderType
{
    ShaderType_Null = 0,
    ShaderType_Vertex,
    ShaderType_Pixel,
    
    ShaderType_Count
};

enum R_InputAssembly
{
	InputAssembly_Triangles,
	InputAssembly_Lines,
	
	InputAssembly_Count,
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
    TextureFormat_DepthStencil,
    
    TextureFormat_Count
};

enum R_TextureChannel
{
    TextureChannel_Zero,
    TextureChannel_One,
    TextureChannel_R,
    TextureChannel_G,
    TextureChannel_B,
    TextureChannel_A,
    
    TextureChannel_Count,
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
    bool scissorEnable = true;
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
struct R_Shader;
struct R_ShaderPack;
struct R_BufferAttribPack;
struct R_Pipeline;
struct R_Texture2D;
struct R_Cubemap;
struct R_Framebuffer;
struct R_Rasterizer;

// ...which are defined here
#ifdef GFX_OPENGL
#include "renderer_backend/opengl.h"
#elif defined(GFX_D3D11)
#include "renderer_backend/d3d11.h"
#else
#error "Unsupported gfx api."
#endif

// Buffers
R_Buffer R_BufferAlloc(R_BufferFlags flags, u32 vertStride);
void R_BufferTransfer(R_Buffer* b, u64 size, void* data);
#define R_BufferTransferStruct(bufferPtr, structValue) R_BufferTransferData(bufferPtr, sizeof(structValue), &structValue)
void R_BufferUpdate(R_Buffer* b, u64 offset, u64 size, void* data);
void R_BufferBind(R_Buffer* b, u32 slot);
void R_BufferFree(R_Buffer* b);

// Shaders
R_Shader R_ShaderAlloc(R_ShaderInput input, R_ShaderType type);
void R_ShaderFree(R_Shader* shader);
R_ShaderPack R_ShaderPackAlloc(R_Shader* shader, u32 shaderCount);
void R_ShaderPackFree(R_ShaderPack* pack);

// Pipelines
R_Pipeline R_PipelineAlloc(R_Pipeline* in, R_InputAssembly assembly); 
void R_PipelineAddBuffer(R_Pipeline* in, R_Buffer* buf);
void R_PipelineBind(R_Pipeline* in);
void R_PipelineFree(R_Pipeline* in);

// Rasterizer state
R_Rasterizer R_RasterizerAlloc(R_RasterizerDesc desc);
void R_RasterizerBind(R_Rasterizer* r);
void R_RasterizerFree(R_Rasterizer* r);

// Textures
R_Texture2D R_Texture2DAlloc(R_TextureFormat format, u32 width, u32 height, void* initData = nullptr,
                             R_TextureUsage usage = TextureUsage_ShaderResource | TextureUsage_Drawable,
                             R_TextureMutability mutability = TextureMutability_Immutable);
void R_Texture2DTransfer(R_Texture2D* t, String data);
void R_Texture2DBind(R_Texture2D* t, u32 slot, R_ShaderType type);
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
void R_CubemapBind(R_Cubemap* c, u32 slot, R_ShaderType type);
void R_CubemapFree(R_Cubemap* c);

// Samplers
R_Sampler R_SamplerAlloc(R_SamplerFilter min = SamplerFilter_LinearMipmapLinear,
                         R_SamplerFilter mag = SamplerFilter_LinearMipmapLinear,
                         R_SamplerWrap wrapU = SamplerWrap_Repeat,
                         R_SamplerWrap wrapV = SamplerWrap_Repeat);
void R_SamplerBind(R_Sampler* s, u32 slot, R_ShaderType type);
void R_SamplerFree(R_Sampler* s);

// Framebuffers
R_Framebuffer R_FramebufferAlloc(u32 width, u32 height, R_Texture2D* colorAttachments,
                                 u32 colorAttachmentsCount, R_Texture2D depthAttachment);
const R_Framebuffer* R_GetScreen();
void R_FramebufferBind(const R_Framebuffer* f);
void R_FramebufferResize(const R_Framebuffer* f, u32 newWidth, u32 newHeight);
void R_FramebufferClear(const R_Framebuffer* f, R_BufferMask mask);
// Unused channels will naturally be ignored
void R_FramebufferFillColorFloat(const R_Framebuffer* f, u32 slot, f64 r, f64 g, f64 b, f64 a);
void R_FramebufferFillColorInt(const R_Framebuffer* f, u32 slot, u64 r, u64 g, u64 b, u64 a);
// NOTE: This causes a stall on the CPU, as it will wait until the data is transferred,
// so try to call this at the end of the frame if necessary
// (0, 0) is located on the bottom left of the image.
// This function returns 0 for unused channels in the corresponding index
IVec4 R_FramebufferReadColor(const R_Framebuffer* f, u32 slot, s32 x, s32 y);
void R_FramebufferFree(R_Framebuffer* f);

// Rendering operations
void R_SetViewport(s32 x, s32 y, s32 w, s32 h);
void R_Draw(R_Pipeline* pipeline, u32 start = 0, u32 count = 0);  // Count = 0 means the entire mesh

// Backend state
void R_Init();  // Initializes the graphics api context
void R_WaitLastFrame();
void R_PresentFrame();
// NOTE: Should be called after R_WaitLastFrame and before any rendering commands
void R_ResizeSwapchain(s32 w, s32 h);
void R_Cleanup();























#if 0

// First we define the backend independent types

struct R_PerSceneData
{
    int test;
};

struct R_PerFrameData
{
    Mat4 world2View;
    Mat4 view2Proj;
    Vec3 viewPos;
};

struct R_PerObjData
{
    Mat4 model2World;
};

R_UniformValue MakeUniformFloat(float value);
R_UniformValue MakeUniformInt(int value);
R_UniformValue MakeUniformUInt(u32 value);
R_UniformValue MakeUniformVec3(Vec3 value);
R_UniformValue MakeUniformVec4(Vec4 value);
R_UniformValue MakeUniformMat4(Mat4 value);

Slice<uchar> MakeUniformBufferStd140(Slice<R_UniformValue> desc, Arena* dst);

// Types defined in the respective .h files:
struct R_Mesh;
struct R_Texture;
struct R_Sampler;
struct R_Framebuffer;
struct R_Shader;
struct R_Pipeline;

void R_SetToDefaultState();

struct ShaderInput
{
    String d3d11Bytecode;
    String dxil;
    String vulkanSpirv;
    String glsl;
};

// Slots for texture/sampler/uniform binding.
// NOTE: These need to be kept in sync with the ones
// in common.hlsli
enum TextureSlot
{
    CodeTex0     = 0,
    CodeTex1     = 1,
    CodeTex2     = 2,
    CodeTex3     = 3,
    CodeTex4     = 4,
    CodeTex5     = 5,
    CodeTex6     = 6,
    CodeTex7     = 7,
    CodeTex8     = 8,
    CodeTex9     = 9,
    MaterialTex0 = 10,
    MaterialTex1 = 11,
    MaterialTex2 = 12,
    MaterialTex3 = 13,
    MaterialTex4 = 14,
    MaterialTex5 = 15,
    MaterialTex6 = 16,
    MaterialTex7 = 17,
    MaterialTex8 = 18,
    MaterialTex9 = 19,
};

enum SamplerSlot
{
    CodeSampler0     = 0,
    CodeSampler1     = 1,
    CodeSampler2     = 2,
    CodeSampler3     = 3,
    CodeSampler4     = 4,
    CodeSampler5     = 5,
    CodeSampler6     = 6,
    CodeSampler7     = 7,
    CodeSampler8     = 8,
    CodeSampler9     = 9,
    MaterialSampler0 = 10,
    MaterialSampler1 = 11,
    MaterialSampler2 = 12,
    MaterialSampler3 = 13,
    MaterialSampler4 = 14,
    MaterialSampler5 = 15,
    MaterialSampler6 = 16,
    MaterialSampler7 = 17,
    MaterialSampler8 = 18,
    MaterialSampler9 = 19,
};

enum CBufferSlot
{
    PerSceneCBuf      = 0,
    PerFrameCBuf      = 1,
    PerObjCBuf        = 2,
    CodeConstants     = 3,
    MaterialConstants = 4,
};

#ifdef GFX_OPENGL
#include "renderer_backend/opengl.h"
#elif defined(GFX_D3D11)
#include "renderer_backend/d3d11.h"
#else
#error "Unsupported gfx api."
#endif

// From this point, these functions are all gfx api dependent

// Initializes the graphics API context
void R_Init();
void R_Cleanup();

void R_ResizeSwapchainIfNecessary();  // Must be called after R_WaitLastFrameAndBeginCurrentFrame
void R_WaitLastFrameAndBeginCurrentFrame();
void R_PresentFrame();

// Utils
// Convert the view to projection matrix based on the API's
// clip space coordinate system
Mat4 R_ConvertClipSpace(Mat4 mat);

// CPU <-> GPU transfers
R_Mesh        R_UploadMesh(Slice<Vertex> verts, Slice<s32> indices);
R_Mesh        R_UploadSkinnedMesh(Slice<AnimVert> verts, Slice<s32> indices);
R_Mesh        R_UploadBasicMesh(Slice<Vec3> verts, Slice<Vec3> normals, Slice<s32> indices);
R_Texture     R_UploadTexture(String blob, u32 width, u32 height, u8 numChannels);
R_Texture     R_UploadCubemap(String top, String bottom, String left, String right, String front, String back, u32 width,
                              u32 height, u8 numChannels);
R_Shader      R_CreateDefaultShader(ShaderKind kind);
R_Mesh        R_CreateDefaultMesh();
R_Shader      R_CreateShader(ShaderKind kind, ShaderInput input);
R_Pipeline    R_CreatePipeline(Slice<R_Shader> shaders);
R_Framebuffer R_DefaultFramebuffer();
R_Framebuffer R_CreateFramebuffer(int width, int height, bool color, R_TextureFormat colorFormat, bool depth, bool stencil);
void          R_ResizeFramebuffer(R_Framebuffer framebuffer, int width, int height);  // Only resizes if necessary

// TODO: Resource destruction
//void DestroyMesh(R_Mesh* mesh);
//void DestroyTexture(R_Texture* tex);

// Drawing
void R_DrawMesh(R_Mesh mesh);
void R_DrawArrow(Vec3 ori, Vec3 dst, float baseRadius, float coneRadius, float coneLength);
void R_DrawSphere(Vec3 center, float radius);
void R_DrawInvertedSphere(Vec3 center, float radius);
void R_DrawCone(Vec3 baseCenter, Vec3 dir, float radius, float length);
void R_DrawCylinder(Vec3 center, Vec3 dir, float radius, float height);
// Counter clockwise is assumed. Both faces face the same way
void R_DrawQuad(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4);
void R_DrawFullscreenQuad();

// Setting state
void R_SetViewport(int width, int height);
void R_SetVertexShader(R_Shader shader);
void R_SetPixelShader(R_Shader shader);
void R_SetFramebuffer(R_Framebuffer framebuffer);
void R_SetTexture(R_Texture texture, ShaderKind kind, TextureSlot slot);
void R_SetSampler(R_SamplerKind samplerKind, ShaderKind kind, SamplerSlot slot);

void R_SetPerSceneData();
void R_SetPerFrameData(Mat4 world2View, Mat4 view2Proj, Vec3 viewPos);
void R_SetPerObjData(Mat4 model2World, Mat3 normalMat);
#define R_SetCodeConstants(shader, desc) R_SetCodeConstants_(desc, __FILE__, __LINE__)
void R_SetCodeConstants_(R_Shader shader, Slice<R_UniformValue> desc, const char* callFile, int callLine);
struct Material;
bool R_CheckMaterial(Material* mat, String matName);
void R_SetMaterialConstants(R_Shader shader, Slice<R_UniformValue> desc);

void R_ClearFrame(Vec4 color);
void R_ClearFrameInt(int r, int g, int b, int a);
void R_ClearDepth();
void R_DepthTest(bool enable);
void R_CullFace(bool enable);
void R_AlphaBlending(bool enable);

// Getting state
R_Texture R_GetFramebufferColorTexture(R_Framebuffer framebuffer);
// Read to CPU, (0, 0) is bottom left and (width, height) top right
int R_ReadIntPixelFromFramebuffer(int x, int y);
Vec4 R_ReadPixelFromFramebuffer(int x, int y);
R_Sampler GetSampler(R_SamplerKind kind);

// Libraries
void R_DearImguiInit();
void R_DearImguiBeginFrame();
void R_DearImguiRender();
void R_DearImguiShutdown();

#endif
