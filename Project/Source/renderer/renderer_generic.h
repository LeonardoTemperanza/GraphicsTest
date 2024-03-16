
#pragma once

struct Particle;

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

// Function pointers
void SetRenderFunctionPointers(OS_GraphicsLib gfxLib);

void StubInitRenderer(Renderer* renderer, Arena* renderArena);
void StubRender(Renderer* r, RenderSettings settings);
void StubCleanup(Renderer* r, Arena* renderArena);

void (*InitRenderer)(Renderer* renderer, Arena* renderArena) = StubInitRenderer;
void (*Render)(Renderer* renderer, RenderSettings settings) = StubRender;
void (*CleanupRenderer)(Renderer* renderer, Arena* renderArena) = StubCleanup;