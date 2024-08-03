
#pragma once

#include "base.h"

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

struct Vertex
{
    Vec3 pos;
    Vec3 normal;
    Vec2 texCoord;
    Vec3 tangent;
};

struct CameraParams
{
    float nearClip;
    float farClip;
    float fov;  // Horizontal, and in degrees
};

// TODO: First draft of what it would look like for
// a skeletal mesh
#define MaxBonesInfluence 4
struct AnimVert
{
    Vec3 pos;
    Vec3 normal;
    Vec2 texCoord;
    Vec3 tangent;
    
    struct BoneWeight
    {
        int boneIdx;
        float weight;
    } boneWeights[MaxBonesInfluence];
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

// Utils
Mat4 R_ConvertView2ProjMatrix(Mat4 mat);

// CPU <-> GPU transfers
R_Buffer   R_UploadMesh(Slice<Vertex> verts, Slice<s32> indices);
R_Buffer   R_UploadSkinnedMesh(Slice<AnimVert> verts, Slice<s32> indices);
R_Texture  R_UploadTexture(String blob, u32 width, u32 height, u8 numChannels);
R_UniformBuffer R_CreateUniformBuffer(u32 binding);
void       R_UploadUniformBuffer(R_UniformBuffer buffer, Slice<R_UniformValue> desc);
R_Shader   R_CompileShader(ShaderKind kind, String dxil, String vulkanSpirv, String glsl);
R_Pipeline R_CreatePipeline(Slice<R_Shader> shaders);
R_Framebuffer R_DefaultFramebuffer();
R_Framebuffer R_CreateFramebuffer(int width, int height, bool color, bool depth, bool stencil);
void R_ResizeFramebuffer(int width, int height);

void R_Init();

// Drawing
void R_DrawModelNoReload(Model* model);
void R_DrawModel(Model* model);

// Setting state
void R_SetViewport(int width, int height);
void R_SetPipeline(R_Pipeline pipeline);
void R_SetUniforms(Slice<R_UniformValue> desc);
void R_SetFramebuffer(R_Framebuffer framebuffer);
void R_ClearFrame(Vec4 color);
void R_EnableDepthTest(bool enable);
void R_EnableCullFace(bool enable);
void R_EnableAlphaBlending(bool enable);
void R_Cleanup();

// Immediate operations
void R_DrawArrow(Vec3 ori, Vec3 dst, float baseRadius, float coneLength, float coneRadius);
void R_DrawCone(Vec3 baseCenter, Vec3 dir, float length, float radius);
void R_DrawCylinder(Vec3 center, float radius, float height);
// Counter clockwise is assumed. Both faces face the same way
void R_DrawQuad(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4);
void R_DrawFullscreenQuad();

// Libraries
void R_RenderDearImgui();
void R_ShutdownDearImgui();