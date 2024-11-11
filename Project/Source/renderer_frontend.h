
#pragma once

#include "base.h"
#include "renderer_backend/generic.h"
#include "serialization.h"

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
void RenderFrame(EntityManager* entities);  // Entrypoint of renderer

void RenderScene(EntityManager* entities, Vec3 camPos, f32 fov, f32 nearClip, f32 farClip);
void RenderOutlines(EntityManager* entities, Vec4 color, f32 thickness = 1.0f);  // Thickness is in pixels
void RenderOutlines(EntityManager* entities);

void DebugDrawArrow();
void DebugDrawLine();
void DebugDrawSphere();
void DebugDrawCube();

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
