
#pragma once

#include "base.h"
#include "serialization.h"

enum ShaderKind;
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

struct CameraParams
{
    float nearClip;
    float farClip;
    float fov;  // Horizontal, and in degrees
};

struct BasicMesh
{
    Slice<Vec3> verts;
    Slice<u32>  indices;
};

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

// Types defined in the respective .h files:
struct R_Mesh;
struct R_Texture;
struct R_Framebuffer;
struct R_Shader;
struct R_Pipeline;
struct Renderer;

#ifdef GFX_OPENGL
#include "renderer_opengl.h"
#elif GFX_D3D12
#include "renderer_d3d12.h"
#endif

// Initializes the graphics API context
void R_Init();
void R_SetToDefaultState();
void R_Cleanup();

// Utils
// Convert the view to projection matrix based on the API's
// clip space coordinate system
Mat4 R_ConvertView2ProjMatrix(Mat4 mat);

// CPU <-> GPU transfers
R_Mesh        R_UploadMesh(Slice<Vertex> verts, Slice<s32> indices);
R_Mesh        R_UploadSkinnedMesh(Slice<AnimVert> verts, Slice<s32> indices);
R_Texture     R_UploadTexture(String blob, u32 width, u32 height, u8 numChannels);
R_Texture     R_UploadCubemap(String top, String bottom, String left, String right, String front, String back, u32 width,
                              u32 height, u8 numChannels);
R_Shader      R_MakeDefaultShader(ShaderKind kind);
R_Mesh        R_MakeDefaultMesh();
R_Shader      R_CompileShader(ShaderKind kind, String dxil, String vulkanSpirv, String glsl);
R_Pipeline    R_CreatePipeline(Slice<R_Shader> shaders);
R_Framebuffer R_DefaultFramebuffer();
R_Framebuffer R_CreateFramebuffer(int width, int height, bool color, R_TextureFormat colorFormat, bool depth, bool stencil);
void          R_ResizeFramebuffer(R_Framebuffer framebuffer, int width, int height);  // Only resizes if necessary

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
void R_SetPipeline(R_Pipeline pipeline);
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

// This is OS dependent. Equivalent of SwapBuffers in functionality
void R_SubmitFrame();

// Libraries
void R_InitDearImgui();
void R_DearImguiBeginFrame();
void R_RenderDearImgui();
void R_ShutdownDearImgui();
