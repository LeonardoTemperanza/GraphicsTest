
#include "renderer_generic.h"

void StubInitRenderer(Renderer* renderer, Arena* renderArena) {};
void StubRender(Renderer* renderer, RenderSettings settings) {};
void StubCleanup(Renderer* renderer, Arena* renderArena) {};

void SetRenderFunctionPointers(OS_GraphicsLib gfxLib)
{
    switch(gfxLib)
    {
        case GfxLib_None:
        {
            InitRenderer = StubInitRenderer;
            Render = StubRender;
            break;
        }
        case GfxLib_OpenGL:
        {
            InitRenderer = gl_InitRenderer;
            Render = gl_Render;
            break;
        }
        case GfxLib_D3D11:
        {
            InitRenderer = d3d11_InitRenderer;
            Render = d3d11_Render;
            break;
        }
        default:
        {
            TODO;
            OS_FatalError("This Graphics API is currently not supported.");
        }
    }
}