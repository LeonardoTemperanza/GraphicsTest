
#include "renderer_frontend.h"

// NOTE: We're assuming that the backend will always use std140 for uniform layout
typedef Vec2Std140 GPUVec2;
typedef Vec3Std140 GPUVec3;
typedef Vec4Std140 GPUVec4;
typedef Mat4       GPUMat4;

struct PerView
{
    GPUMat4 world2View;
    GPUMat4 viewToProj;
    GPUVec3 viewPos;
};

static RenderResources renderResources;
