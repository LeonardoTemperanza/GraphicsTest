
#pragma once

#include "base.h"

struct Camera;
struct Model;
struct Texture;
struct Entities;
enum ShaderKind;

// Used for GPU resources
typedef u32 R_Texture;
typedef u32 R_Shader;
typedef u32 R_Program;
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

R_Buffer  R_UploadMesh(Slice<Vertex> verts, Slice<s32> indices);
R_Buffer  R_UploadSkinnedMesh(Slice<AnimVert> verts, Slice<s32> indices);
R_Texture R_UploadTexture(String blob, u32 width, u32 height, u8 numChannels);
R_Shader  R_CompileShader(ShaderKind kind, String dxil, String vulkanSpirv, String glsl);
R_Program R_LinkShaders(Slice<R_Shader> shaders);

void R_Init();
void R_BeginPass(Camera* entities);
void R_DrawModelNoReload(Model* model, Mat4 transform);
void R_DrawModel(Model* model, Mat4 transform);
void R_Cleanup();
