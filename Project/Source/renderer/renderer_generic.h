
#pragma once

#include "base.h"
#include "asset_system.h"

struct RenderSettings
{
    Transform camera;
    float horizontalFOV;
    float nearClipPlane;
    float farClipPlane;
};

struct Entity;

// Function pointers
#define InitRenderer_Signature(name) void name()
typedef InitRenderer_Signature(InitRenderer_Type);
#define Render_Signature(name) void name(Slice<Entity> entities, RenderSettings renderSettings)
typedef Render_Signature(Render_Type);
#define SetupGPUResources_Signature(name) void name(Model* model, Arena* arena)
typedef SetupGPUResources_Signature(SetupGPUResources_Type);
#define RenderModelRelease_Signature(name) void name(Model* model)
typedef RenderModelRelease_Signature(RenderModelRelease_Type);
#define Cleanup_Signature(name) void name()
typedef Cleanup_Signature(Cleanup_Type);

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

void SetRenderFunctionPointers(OS_GraphicsLib gfxLib);
Model* LoadModel(const char* path);
Model* LoadModelByName(const char* name);
void RenderModelDevelopment(Model* model);

extern InitRenderer_Type* InitRenderer;
extern Render_Type* Render;
extern SetupGPUResources_Type* SetupGPUResources;
extern RenderModelRelease_Type* RenderModelRelease;
extern Cleanup_Type* Cleanup;

#ifdef Development
#define RenderModel(model) RenderModelDevelopment(model)
#else
#define RenderModel(model) RenderModelRelease(model)
#endif
