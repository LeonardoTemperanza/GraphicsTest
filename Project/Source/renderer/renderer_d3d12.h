
#pragma once

union Renderer;

struct d3d11_Renderer
{
    ID3D11Texture2D* framebuffer;
    ID3D11Texture2D* depthBuffer;
};

InitRenderer_Signature(d3d11_InitRenderer);
Render_Signature(d3d11_Render);

// For syntax highlighting
#if 0
void d3d11_InitRenderer();
void d3d11_Render();
#endif
