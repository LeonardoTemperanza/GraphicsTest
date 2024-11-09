
#pragma once

#include "base.h"
#include "renderer_backend/generic.h"

struct RenderResources
{
    // Samplers
    R_Sampler* common;  // Linear sampler used for most things
    
    // Render passes
    R_Framebuffer* directionalShadows;
    
    // Editor
    R_Framebuffer* entityIds;
    R_Framebuffer* outlines;
};

struct Model
{
    R_Buffer* verts;
    R_Buffer* indices;
};

void RenderScene(EntityManager* entities);
void RenderOutlines(EntityManager* entities, Vec4 color, f32 thickness = 1.0f);  // Thickness is in pixels
void RenderOutlines(EntityManager* entities);
void RenderResourcesCleanup();

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
