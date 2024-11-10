
#pragma once

#include "base.h"
#include "renderer_backend/generic.h"

struct Mesh
{
    R_Buffer vertBuffer;
    R_Buffer idxBuffer;
};

struct Model
{
    Array<Mesh> meshes;
};

struct MeshInput
{
    Slice<Vertex> verts;
    Slice<u32> indices;
};

Model ModelAlloc(Slice<MeshInput> meshes);
void ModelFree(Model* model);

struct RenderResources
{
    // Samplers
    R_Sampler common;  // Linear sampler used for most things
    
    // Render passes
    R_Framebuffer directionalShadows;
    
    // Editor
    R_Texture selectionColor;
    R_Texture selectionDepth;
    R_Framebuffer selectionBuffer;
    R_Texture outlinesColor;
    R_Texture outlinesDepth;
    R_Framebuffer outlines;
    
    // Assets
    //CubemapHandle skybox;
};

void RenderResourcesInit();
void UpdateFramebuffers();
void RenderResourcesCleanup();

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
