
#pragma once

#include "renderer_generic.h"

struct d3d11_Renderer
{
    ID3D11Texture2D* framebuffer;
    ID3D11Texture2D* depthBuffer;
};

d3d11_Renderer* d3d11_InitRenderer(Arena* permArena);
void d3d11_Render(d3d11_Renderer* r, RenderSettings renderSettings);
