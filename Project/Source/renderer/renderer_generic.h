
#pragma once

#include "asset_system.h"

struct RenderSettings
{
    Transform camera;
    float horizontalFOV;
    float nearClipPlane;
    float farClipPlane;
    
    Slice<Particle> particles;
};

#ifdef OS_SupportOpenGL
#include "renderer_opengl.h"
#endif
#ifdef OS_SupportD3D11
#include "renderer_d3d11.h"
#endif

union Renderer
{
#ifdef OS_SupportOpenGL
    gl_Renderer glRenderer;
#endif
#ifdef OS_SupportD3D11
    d3d11_Renderer d3d11Renderer;
#endif
};

typedef u32 MeshGfxHandles;

#if 0
union MeshGfxHandles
{
#ifdef OS_SupportOpenGL
    gl_Mesh glMesh;
#endif
#ifdef OS_SupportD3D11
    d3d11_Mesh d3d11Mesh;
#endif
};
#endif

void SetRenderFunctionPointers(OS_GraphicsLib gfxLib);
Model* LoadModel(const char* path);
Model* LoadModelByName(const char* name);
void RenderModelDevelopment(Model* model);

// Function pointers. Macros could be used here to save some time
void StubInitRenderer();
void StubRender(RenderSettings settings);
void StubCreateGPUBuffers(Model* model);
void StubRenderModel(Model* model);
void StubCleanup();

void (*InitRenderer)() = StubInitRenderer;
void (*Render)(RenderSettings settings) = StubRender;
void (*CreateGPUBuffers)(Model* model) = StubCreateGPUBuffers;
void (*RenderModelRelease)(Model* model) = StubRenderModel;
void (*CleanupRenderer)() = StubCleanup;

#ifdef Development
#define RenderModel(model) RenderModelDevelopment(model)
#else
#define RenderModel(model) RenderModelRelease(model)
#endif