
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

#include "renderer_opengl.h"
#include "renderer_d3d11.h"

// Function pointers
void SetRenderFunctionPointers(OS_GraphicsLib gfxLib);

typedef void* RendererRef;

RendererRef StubInitRenderer(Arena* arena);
void StubRender(RendererRef r, RenderSettings settings);

RendererRef (*InitRenderer)(Arena* arena) = StubInitRenderer;
void (*Render)(RendererRef r, RenderSettings settings) = StubRender;

// For text editor syntax highlighting
#if 0
void InitRenderer();
void Render();
#endif