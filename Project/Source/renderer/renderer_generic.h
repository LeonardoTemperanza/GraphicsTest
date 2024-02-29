
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

struct Renderer
{
    union
    {
        gl_Renderer glRenderer;
        d3d11_Renderer d3d11Renderer;
    };
};

Renderer StubInitRenderer(Arena* arena);
void StubRender(Renderer* r, RenderSettings settings);

Renderer (*InitRenderer)(Arena* arena) = StubInitRenderer;
void (*Render)(Renderer* r, RenderSettings settings) = StubRender;