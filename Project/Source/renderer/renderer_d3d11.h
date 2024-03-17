
#pragma once

union Renderer;

struct d3d11_Renderer
{
    ID3D11Texture2D* framebuffer;
    ID3D11Texture2D* depthBuffer;
};

void d3d11_InitRenderer();
void d3d11_Render(RenderSettings renderSettings);
