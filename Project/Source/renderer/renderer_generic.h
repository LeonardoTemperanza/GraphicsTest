
#pragma once

#include "base.h"
#include "serialization.h"

// First we define the backend independent types

enum UniformType;
struct R_UniformValue
{
    UniformType type;
    union
    {
        float f;
        int i;
        u32 ui;
        Mat4 mat4;
        Vec3 v3;
        Vec4 v4;
    } value;
};

struct R_CameraParams
{
    float nearClip;
    float farClip;
    float fov;  // Horizontal, and in degrees
};

enum R_MeshKind
{
    R_BasicMesh = 0,
    R_StaticMesh,
    R_SkinnedMesh
};

struct R_BasicMesh
{
    Slice<Vec3> verts;
    Slice<u32>  indices;
};

struct R_Mesh;
R_Mesh R_GenerateUnitCylinderMesh();
R_Mesh R_GenerateUnitConeMesh();

enum R_TextureKind
{
    R_Tex2D = 0,
    R_TexCubemap,
};

enum R_TextureFormat
{
    R_TexNone = 0,
    R_TexR8,
    R_TexRG8,
    R_TexRGB8,
    R_TexRGBA8,
    R_TexR8I,
    R_TexR8UI,
    R_TexR32I,
};

bool IsTextureFormatSigned(R_TextureFormat format);
bool IsTextureFormatInteger(R_TextureFormat format);

// NOTE: For now we just have a select number of samplers
// for now i think it's fine. I don't think we'll need that many, but we'll see
// We're not trying to build a completely generic interface here, we're trying
// to build an abstraction that is useful for this specific usecase.
enum R_SamplerKind
{
    R_SamplerDefault,
    R_SamplerShadow,
    
    R_SamplerCount
};

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

#ifdef GFX_OPENGL
#include "renderer_opengl.h"
#elif defined(GFX_D3D11)
#include "renderer_d3d11.h"
#else
#error "Unsupported gfx api."
#endif

// From this point, these functions are all gfx api dependent

// Initializes the graphics API context
void R_Init();
void R_Cleanup();

// Utils
// Convert the view to projection matrix based on the API's
// clip space coordinate system
Mat4 R_ConvertView2ProjMatrix(Mat4 mat);

// CPU <-> GPU transfers
R_Mesh        R_UploadMesh(Slice<Vertex> verts, Slice<s32> indices);
R_Mesh        R_UploadSkinnedMesh(Slice<AnimVert> verts, Slice<s32> indices);
R_Mesh        R_UploadBasicMesh(Slice<Vec3> verts, Slice<Vec3> normals, Slice<s32> indices);
R_Texture     R_UploadTexture(String blob, u32 width, u32 height, u8 numChannels);
R_Texture     R_UploadCubemap(String top, String bottom, String left, String right, String front, String back, u32 width,
                              u32 height, u8 numChannels);
R_Shader      R_CreateDefaultShader(ShaderKind kind);
R_Mesh        R_CreateDefaultMesh();
R_Shader      R_CompileShader(ShaderKind kind, ShaderInput input);
R_Pipeline    R_CreatePipeline(Slice<R_Shader> shaders);
R_Framebuffer R_DefaultFramebuffer();
R_Framebuffer R_CreateFramebuffer(int width, int height, bool color, R_TextureFormat colorFormat, bool depth, bool stencil);
void          R_ResizeFramebuffer(R_Framebuffer framebuffer, int width, int height);  // Only resizes if necessary

// Drawing
// TODO: Mhmmm... this is kind of weird because it's different from any other resource.
// Should we just have a "R_SetMesh" and "R_DrawCall" ?
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
void R_SetUniforms(Slice<R_UniformValue> desc);
void R_SetFramebuffer(R_Framebuffer framebuffer);
void R_SetTexture(R_Texture texture, u32 slot);

void R_SetPerSceneData();
void R_SetPerFrameData(Mat4 world2View, Mat4 view2Proj, Vec3 viewPos);
void R_SetPerObjData(Mat4 model2World, Mat3 normalMat);

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

void R_WaitLastFrameAndBeginCurrentFrame();
void R_ResizeMainFramebufferIfNecessary();
void R_SubmitFrame();

// Libraries
void R_DearImguiInit();
void R_DearImguiBeginFrame();
void R_DearImguiRender();
void R_DearImguiShutdown();
