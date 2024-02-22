
#pragma once

#include "renderer_generic.h"

struct d3d11_Renderer
{
    int a;
};

d3d11_Renderer d3d11_InitRenderer(Arena* permArena);
void d3d11_Render(d3d11_Renderer* r, RenderSettings renderSettings);
