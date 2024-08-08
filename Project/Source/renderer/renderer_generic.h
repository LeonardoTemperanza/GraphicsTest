
#pragma once

#include "base.h"
#include "serialization.h"

struct Model;
struct Texture;
struct Entities;
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

enum R_TextureFormat
{
    R_TexNone = 0,
    R_TexRGB8,
    R_TexRGBA8,
    R_TexR8I,
    R_TexR8UI,
    R_TexR32I,
};

struct BasicMesh
{
    Slice<Vec3> verts;
    Slice<u32>  indices;
};

R_UniformValue MakeUniformFloat(float value);
R_UniformValue MakeUniformInt(int value);
R_UniformValue MakeUniformUInt(u32 value);
R_UniformValue MakeUniformVec3(Vec3 value);
R_UniformValue MakeUniformVec4(Vec4 value);
R_UniformValue MakeUniformMat4(Mat4 value);

#ifdef GFX_OPENGL
#include "renderer_opengl.h"
#elif GFX_D3D12
#include "renderer_d3d12.h"
#endif

// Initializes the graphics API context
void R_Init();
void R_Cleanup();

// Utils
// Convert the view to projection matrix based on the API's
// Clip space coordinate system
Mat4 R_ConvertView2ProjMatrix(Mat4 mat);

// CPU <-> GPU transfers
R_Buffer        R_UploadMesh(Slice<Vertex> verts, Slice<s32> indices);
R_Buffer        R_UploadSkinnedMesh(Slice<AnimVert> verts, Slice<s32> indices);
R_Texture       R_UploadTexture(String blob, u32 width, u32 height, u8 numChannels);
R_UniformBuffer R_CreateUniformBuffer(u32 binding);
void            R_UploadUniformBuffer(R_UniformBuffer buffer, Slice<R_UniformValue> desc);
R_Shader        R_CompileShader(ShaderKind kind, String dxil, String vulkanSpirv, String glsl);
R_Pipeline      R_CreatePipeline(Slice<R_Shader> shaders);
R_Framebuffer   R_DefaultFramebuffer();
R_Framebuffer   R_CreateFramebuffer(int width, int height, bool color, R_TextureFormat colorFormat, bool depth, bool stencil);
void            R_ResizeFramebuffer(R_Framebuffer framebuffer, int width, int height);  // Only resizes if necessary

// Drawing
void R_DrawModel(Model* model);
void R_DrawModelNoReload(Model* model);
void R_DrawArrow(Vec3 ori, Vec3 dst, float baseRadius, float coneLength, float coneRadius);
void R_DrawCone(Vec3 baseCenter, Vec3 dir, float length, float radius);
void R_DrawCylinder(Vec3 center, float radius, float height);
// Counter clockwise is assumed. Both faces face the same way
void R_DrawQuad(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4);
void R_DrawFullscreenQuad();

// Setting state
void R_SetViewport(int width, int height);
void R_SetPipeline(R_Pipeline pipeline);
void R_SetUniforms(Slice<R_UniformValue> desc);
void R_SetFramebuffer(R_Framebuffer framebuffer);
void R_SetTexture(R_Texture texture, u32 slot);

void R_ClearFrame(Vec4 color);
void R_ClearFrameInt(int r, int g, int b, int a);
void R_DepthTest(bool enable);
void R_CullFace(bool enable);
void R_AlphaBlending(bool enable);

// Getting state
R_Texture R_GetFramebufferColorTexture(R_Framebuffer framebuffer);
// Read to CPU, (0, 0) is bottom left and (width, height) top right
int R_ReadIntPixelFromFramebuffer(int x, int y);
int R_ReadIntPixelFromFramebufferAndGetItNextFrame(int x, int y);
Vec4 R_ReadPixelFromFramebuffer(int x, int y);
Vec4 R_ReadPixelFromFramebufferAndGetItNextFrame(int x, int y);

// Libraries
void R_RenderDearImgui();
void R_ShutdownDearImgui();
