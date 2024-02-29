
#include "renderer_generic.h"

Renderer StubInitRenderer(Arena* arena) { return {0}; }
void StubRender(Renderer* r, RenderSettings settings) {};

void SetRenderFunctionPointers(OS_GraphicsLib gfxLib)
{
#define CastToGenericInit   (Renderer (*)(Arena*))
#define CastToGenericRender (void (*)(Renderer* r, RenderSettings settings))
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
            InitRenderer = CastToGenericInit gl_InitRenderer;
            Render = CastToGenericRender gl_Render;
            break;
        }
        case GfxLib_D3D11:
        {
            InitRenderer = CastToGenericInit d3d11_InitRenderer;
            Render = CastToGenericRender d3d11_Render;
            break;
        }
        default:
        {
            TODO;
            OS_FatalError("This Graphics API is currently not supported.");
        }
    }
#undef CastToGenericInit
#undef CastToGenericRender
}