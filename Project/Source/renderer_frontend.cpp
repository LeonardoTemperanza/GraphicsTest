
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

// NOTE: These need to be updated along with the ones in common.hlsli
enum TextureSlot
{
    CodeTex0 = 0,
    CodeTex1,
    CodeTex2,
    CodeTex3,
    CodeTex4,
    CodeTex5,
    CodeTex6,
    CodeTex7,
    CodeTex8,
    CodeTex9,
    MatTex0,
    MatTex1,
    MatTex2,
    MatTex3,
    MatTex4,
    MatTex5,
    MatTex6,
    MatTex7,
    MatTex8,
    MatTex9,
};

enum SamplerSlot
{
    CodeSampler0 = 0,
    CodeSampler1,
    CodeSampler2,
    CodeSampler3,
    CodeSampler4,
    CodeSampler5,
    CodeSampler6,
    CodeSampler7,
    CodeSampler8,
    CodeSampler9,
};

enum CBufferSlot
{
    PerScene = 0,
    PerFrame = 1,
    PerObj   = 2,
    CodeConstants = 3,
    MatConstants = 4
};

static RenderResources renderResources;

void RenderResourcesInit()
{
    auto& res = renderResources;
    
    s32 w, h;
    OS_GetClientAreaSize(&w, &h);
    
    res.common = R_SamplerAlloc();
    
    res.selectionColor = R_Texture2DAlloc(TextureFormat_R32Int, w, h);
    res.selectionDepth = R_Texture2DAlloc(TextureFormat_DepthStencil, w, h);
    res.selectionBuffer  = R_FramebufferAlloc(w, h, &res.selectionColor, 1, res.selectionDepth);
    
    res.outlinesColor = R_Texture2DAlloc(TextureFormat_R32Int, w, h);
    res.outlinesDepth = R_Texture2DAlloc(TextureFormat_DepthStencil, w, h);
    res.outlines      = R_FramebufferAlloc(w, h, &res.outlinesColor, 1, res.outlinesDepth);
    
    res.skybox = AcquireCubemap("Skybox/sky2.png");
}

void UpdateFramebuffers()
{
    auto& res = renderResources;
    
    s32 w, h;
    OS_GetClientAreaSize(&w, &h);
    
    R_FramebufferResize(&res.selectionBuffer, w, h);
    R_FramebufferResize(&res.outlines, w, h);
}

void RenderResourcesCleanup()
{
    auto& res = renderResources;
    
    R_SamplerFree(&res.common);
    
    R_Texture2DFree(&res.selectionColor);
    R_Texture2DFree(&res.selectionDepth);
    R_FramebufferFree(&res.selectionBuffer);
    
    R_Texture2DFree(&res.outlinesColor);
    R_Texture2DFree(&res.outlinesDepth);
    R_FramebufferFree(&res.outlines);
    
    ReleaseCubemap(res.skybox);
}

void RenderFrame(EntityManager* entities)
{
    UpdateFramebuffers();
    
    const R_Framebuffer* screen = R_GetScreen();
    R_FramebufferClear(screen, BufferMask_Depth & BufferMask_Stencil);
    R_FramebufferFillColorFloat(screen, 0, 0.5f, 0.5f, 0.5f, 1.0f);
    
    R_PresentFrame();
}
