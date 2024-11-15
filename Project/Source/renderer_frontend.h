
#pragma once

#include "base.h"
#include "renderer_backend/generic.h"
#include "serialization.h"

struct CamParams
{
    Vec3 pos;
    Quat rot;
    float fov;
    float nearClip;
    float farClip;
};

struct Mesh
{
    R_Buffer vertBuffer;
    R_Buffer idxBuffer;
};

struct StaticMeshInput
{
    Slice<Vertex> verts;
    Slice<u32> indices;
};

struct SkinnedMeshInput
{
    Slice<AnimVert> animVert;
};

Mesh StaticMeshAlloc(StaticMeshInput input);
Mesh SkinnedMeshAlloc(StaticMeshInput input);
void DrawMesh(Mesh* mesh);
void MeshFree(Mesh* mesh);

// The asset system needs to know what a mesh is
#include "asset_system.h"

void RenderResourcesInit();
void UpdateFramebuffers();
void RenderResourcesCleanup();

struct EntityManager;
void RenderFrame(EntityManager* entities, CamParams cam);  // Entrypoint of renderer

void RenderScene(EntityManager* entities, Vec3 camPos, f32 fov, f32 nearClip, f32 farClip);
void RenderOutlines(EntityManager* entities, Vec4 color, f32 thickness = 1.0f);  // Thickness is in pixels
void RenderOutlines(EntityManager* entities);

// Immediate rendering utilities
struct ImmVertex
{
    Vec3 position;
    float colorScale;
    Vec3 normal;
    Vec2 uv0, uv1;
};

void ImmDrawQuad(Vec4 p0, Vec4 p1, Vec4 p2, Vec4 p3, Vec4 color0, Vec4 color1, Vec4 color2, Vec4 color3);

// Rendering settings
enum AntialiasingType
{
    AA_None,
    AA_MSAA
};

enum ShadowQuality
{
    Shadows_Low,
    Shadows_Mid,
    Shadows_High
};

struct GraphicsSettings
{
    bool vsync;
    AntialiasingType aa;
};

void SetGraphicsSettings(GraphicsSettings settings);
