
#pragma once

#include "base.h"

struct Model;
struct Texture;
struct Entities;
enum ShaderKind;
enum UniformType;

// Used for GPU resources
typedef u32 R_Texture;
typedef u32 R_Shader;
typedef u32 R_Pipeline;
typedef u32 R_Buffer;

struct Vertex
{
    Vec3 pos;
    Vec3 normal;
    Vec2 texCoord;
    Vec3 tangent;
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

#ifdef GFX_OPENGL
#include "renderer_opengl.h"
#elif GFX_D3D12
#include "renderer_d3d12.h"
#endif

// CPU <-> GPU transfers
R_Buffer   R_UploadMesh(Slice<Vertex> verts, Slice<s32> indices);
R_Buffer   R_UploadSkinnedMesh(Slice<AnimVert> verts, Slice<s32> indices);
R_Texture  R_UploadTexture(String blob, u32 width, u32 height, u8 numChannels);
R_Shader   R_CompileShader(ShaderKind kind, String dxil, String vulkanSpirv, String glsl);
R_Pipeline R_LinkShaders(Slice<R_Shader> shaders);

void R_Init();
void R_BeginPass(Vec3 camPos, Quat camRot, float horizontalFOV, float nearClip, float farClip);
void R_DrawModelNoReload(Model* model, Mat4 transform, int id = -1);
void R_DrawModel(Model* model, Mat4 transform, int id = -1);  // Id for entity selection
#ifdef Development
void R_DrawModelEditor(Model* model, Mat4 transform, int id = -1, bool selected = false);
void R_DrawSelectionOutlines(Vec4 color);
#endif
void R_SetPipeline(R_Pipeline pipeline);
void R_SetUniformFloat(u32 binding, float value);
void R_SetUniformInt(u32 binding, int value);
void R_SetUniformVec3(u32 binding, Vec3 value);
void R_SetUniformVec4(u32 binding, Vec4 value);
void R_SetUniformMat4(u32 binding, Mat4 value);
template<typename t>
void R_SetUniformNamed(const char* name, t value);
void R_SetUniformBuffer(u32 binding, void* buffer);
void R_SetUniformBufferNamed(const char* name, void* buffer);
void R_Cleanup();

// Utils
#ifdef Development
int R_ReadMousePickId(int xPos, int yPos);  // Returns -1 if no id was picked
#endif

// Immediate operations
void R_ImDrawArrow(Vec3 ori, Vec3 dst, float baseRadius, float coneLength, float coneRadius, Vec4 color);
void R_ImDrawCone(Vec3 baseCenter, Vec3 dir, float length, float radius, Vec4 color);
void R_ImDrawCylinder(Vec3 center, float radius, float height, Vec4 color);
// Counter clockwise is assumed. Both faces face the same way
void R_ImDrawQuad(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4, Vec4 color);

// Libraries
void R_RenderDearImgui();
void R_ShutdownDearImgui();
void R_ImGuiShowDebugTextures();