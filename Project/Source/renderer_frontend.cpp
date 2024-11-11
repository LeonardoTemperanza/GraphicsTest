
#include "renderer_frontend.h"
#include "renderer_backend/generic.h"

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

struct RenderResources
{
    // Vertex layouts
    R_VertLayout staticLayout;
    R_VertLayout skinnedLayout;
    
    // Samplers
    R_Sampler commonSampler;  // Linear sampler used for most things
    
    // Render passes
    R_Framebuffer directionalShadows;
    
    // Editor
#if 0
    R_Texture selectionColor;
    R_Texture selectionDepth;
    R_Framebuffer selectionBuffer;
    R_Texture outlinesColor;
    R_Texture outlinesDepth;
    R_Framebuffer outlines;
#endif
    
    // Assets
    //CubemapHandle skybox;
    VertShaderHandle staticVertShader;
    PixelShaderHandle simplePixelShader;
};

static RenderResources renderResources;

Mesh StaticMeshAlloc(StaticMeshInput input)
{
    Mesh res = {};
    res.vertBuffer = R_BufferAlloc(BufferFlag_Vertex, sizeof(Vertex), input.verts.len * sizeof(Vertex), input.verts.ptr);
    res.idxBuffer  = R_BufferAlloc(BufferFlag_Index, 4, input.indices.len * 4, input.indices.ptr);
    return res;
}

void DrawMesh(Mesh* mesh)
{
    R_Draw(&mesh->vertBuffer, &mesh->idxBuffer, 0, 0);
}

void MeshFree(Mesh* mesh)
{
    R_BufferFree(&mesh->vertBuffer);
    R_BufferFree(&mesh->idxBuffer);
}

void RenderResourcesInit()
{
    auto& res = renderResources;
    
    s32 w, h;
    OS_GetClientAreaSize(&w, &h);
    
    {
        R_VertAttrib attribs[] =
        {
            { .type=VertAttrib_Pos, .bufferSlot=0, .offset=offsetof(Vertex, pos), },
            { .type=VertAttrib_Normal, .bufferSlot=0, .offset=offsetof(Vertex, normal), },
            { .type=VertAttrib_TexCoord, .bufferSlot=0, .offset=offsetof(Vertex, texCoord), },
            { .type=VertAttrib_Tangent, .bufferSlot=0, .offset=offsetof(Vertex, tangent), }
        };
        res.staticLayout = R_VertLayoutAlloc(attribs, ArrayCount(attribs));
    }
    
    {
        R_VertAttrib attribs[] =
        {
            { .type=VertAttrib_Pos, .bufferSlot=0, .offset=offsetof(Vertex, pos), },
            { .type=VertAttrib_Normal, .bufferSlot=0, .offset=offsetof(Vertex, normal), },
            { .type=VertAttrib_TexCoord, .bufferSlot=0, .offset=offsetof(Vertex, texCoord), },
            { .type=VertAttrib_Tangent, .bufferSlot=0, .offset=offsetof(Vertex, tangent), }
        };
        res.skinnedLayout = R_VertLayoutAlloc(attribs, ArrayCount(attribs));
    }
    
#if 0
    res.commonSampler = R_SamplerAlloc();
    
    res.selectionColor = R_Texture2DAlloc(TextureFormat_R32Int, w, h);
    res.selectionDepth = R_Texture2DAlloc(TextureFormat_DepthStencil, w, h);
    res.selectionBuffer  = R_FramebufferAlloc(w, h, &res.selectionColor, 1, res.selectionDepth);
    
    res.outlinesColor = R_Texture2DAlloc(TextureFormat_R32Int, w, h);
    res.outlinesDepth = R_Texture2DAlloc(TextureFormat_DepthStencil, w, h);
    res.outlines      = R_FramebufferAlloc(w, h, &res.outlinesColor, 1, res.outlinesDepth);
    
    //res.skybox = AcquireCubemap("Skybox/sky2.png");
#endif
    
    res.staticVertShader = AcquireVertShader("CompiledShaders/model2proj.shader");
    res.simplePixelShader = AcquirePixelShader("CompiledShaders/paint_red.shader");
    
    const R_Framebuffer* screen = R_GetScreen();
    R_FramebufferBind(screen);
}

void UpdateFramebuffers()
{
    auto& res = renderResources;
    
    s32 w, h;
    OS_GetClientAreaSize(&w, &h);
    
#if 0
    R_FramebufferResize(&res.selectionBuffer, w, h);
    R_FramebufferResize(&res.outlines, w, h);
#endif
}

void RenderResourcesCleanup()
{
    // Not a priority right now. These resources are supposed to
    // live as long as the program does, and the os will clean up
    // these for you, so right now we don't worry too much.
    
#if 0
    auto& res = renderResources;
    
    R_VertLayoutFree(&res.staticLayout);
    R_VertLayoutFree(&res.skinnedLayout);
    
    R_SamplerFree(&res.commonSampler);
    
    R_Texture2DFree(&res.selectionColor);
    R_Texture2DFree(&res.selectionDepth);
    R_FramebufferFree(&res.selectionBuffer);
    
    R_Texture2DFree(&res.outlinesColor);
    R_Texture2DFree(&res.outlinesDepth);
    R_FramebufferFree(&res.outlines);
    
    //ReleaseCubemap(res.skybox);
#endif
}

void RenderFrame(EntityManager* entities)
{
    auto& res = renderResources;
    
    const R_Framebuffer* screen = R_GetScreen();
    R_FramebufferClear(screen, BufferMask_Depth | BufferMask_Stencil);
    R_FramebufferFillColorFloat(screen, 0, 0.5f, 0.5f, 0.5f, 1.0f);
    
    R_ShaderBind(GetAsset(res.staticVertShader));
    R_ShaderBind(GetAsset(res.simplePixelShader));
    
    R_FramebufferBind(R_GetScreen());
    
    {
        R_RasterizerDesc desc = {};
        desc.depthClipEnable = true;
        desc.cullMode = CullMode_Back;
        static R_Rasterizer rasterizer = R_RasterizerAlloc(desc);
        R_RasterizerBind(&rasterizer);
    }
    
    {
        R_DepthDesc desc = {};
        desc.depthEnable = true;
        
        static R_DepthState depthState = R_DepthStateAlloc(desc);
        R_DepthStateBind(&depthState);
    }
    
    R_VertLayoutBind(&res.staticLayout);
    
    // Draw entities
    for_live_entities(entities, ent)
    {
        DrawMesh(GetAsset(ent->mesh));
    }
    
    R_ImGuiDrawFrame();
    
    R_PresentFrame();
}