
#include "renderer_generic.h"

static Renderer renderer;

#ifdef GFX_OPENGL
#include "renderer/renderer_opengl.cpp"
#endif
#ifdef GFX_DX3D11
#include "renderer/renderer_d3d11.cpp"
#endif
