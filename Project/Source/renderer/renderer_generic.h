
#pragma once

#include "base.h"
#include "asset_system.h"

struct RenderSettings
{
    Transform camera;
    float horizontalFOV;
    float nearClipPlane;
    float farClipPlane;
};

struct Entity;

#ifdef GFX_OPENGL
#include "renderer_opengl.h"
#elif GFX_D3D11
#include "renderer_d3d11.h"
#endif

void R_Init();
void R_BeginPass(RenderSettings settings);
void R_DrawModel(Model* model, Vec3 pos, Quat rot, Vec3 scale);
void R_TransferModel(Model* model, Arena* arena);
void R_Cleanup();
