
#include "renderer_frontend.h"
#include "renderer_backend/generic.h"

// NOTE: We're assuming that the backend will always use std140 for uniform layout
struct PerView
{
    alignas(16) Mat4 world2View;
    alignas(16) Mat4 view2Proj;
    alignas(16) Vec4 viewPos;
};

struct PerObj
{
    alignas(16) Mat4 model2World;
    alignas(16) Mat4 normalMat; 
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
    PerSceneSlot      = 0,
    PerViewSlot       = 1,
    PerObjSlot        = 2,
    CodeConstantsSlot = 3,
    MatConstantsSlot  = 4
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
    
    // Uniform buffers
    R_Buffer perView;
    R_Buffer perObj;
    
    // Assets
    //CubemapHandle skybox;
    VertShaderHandle staticVertShader;
    PixelShaderHandle simplePixelShader;
};

static RenderResources renderResources;
static GraphicsSettings gfxSettings;

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

static void UseMaterial(Material* mat)
{
    R_ShaderBind(GetAsset(mat->shader));
    
    for(int i = 0; i < mat->textures.len; ++i)
    {
        R_Texture2DBind(GetAsset(mat->textures[i]), MatTex0 + i, ShaderType_Pixel);
    }
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
    
    res.commonSampler = R_SamplerAlloc();
    
#if 0
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
    
    PerView perView = {};
    perView.world2View = Mat4::identity;
    perView.view2Proj = Mat4::identity;
    
    PerObj perObj = {};
    perObj.model2World = Mat4::identity;
    res.perView = R_BufferAlloc(BufferFlag_Dynamic | BufferFlag_ConstantBuffer, sizeof(PerView), sizeof(PerView), &perView);
    res.perObj  = R_BufferAlloc(BufferFlag_Dynamic | BufferFlag_ConstantBuffer, sizeof(PerObj), sizeof(PerObj), &perObj);
    
    R_BufferUniformBind(&res.perObj,  PerObjSlot,  ShaderType_Vertex);
    R_BufferUniformBind(&res.perView, PerViewSlot, ShaderType_Vertex);
    
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
    // live as long as the program does, and the os will clean these
    // up for you, so right now we don't worry too much.
    
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

void RenderFrame(EntityManager* entities, CamParams cam)
{
    auto& res = renderResources;
    
    const R_Framebuffer* screen = R_GetScreen();
    R_FramebufferClear(screen, BufferMask_Depth | BufferMask_Stencil);
    R_FramebufferFillColorFloat(screen, 0, 0.5f, 0.5f, 0.5f, 1.0f);
    
    R_ShaderBind(GetAsset(res.staticVertShader));
    //R_ShaderBind(GetAsset(res.simplePixelShader));
    
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
    R_BufferUniformBind(&res.perView, PerViewSlot, ShaderType_Vertex);
    
    s32 w, h;
    OS_GetClientAreaSize(&w, &h);
    
    auto view2Proj = View2ProjPerspectiveMatrix(cam.nearClip, cam.farClip, cam.fov, (float)w, (float)h);
    
    PerView perView = {};
    perView.world2View = World2ViewMatrix(cam.pos, cam.rot);
    perView.view2Proj  = R_ConvertClipSpace(view2Proj);
    R_BufferUpdateStruct(&res.perView, perView);
    for_live_entities(entities, ent)
    {
        if(ent->flags & EntityFlags_NoMesh) continue;
        
        PerObj perObj = {};
        perObj.model2World = ComputeWorldTransform(entities, ent);
        perObj.normalMat = transpose(ComputeTransformInverse(perObj.model2World));
        R_BufferUpdateStruct(&res.perObj, perObj);
        
        R_SamplerBind(&res.commonSampler, CodeSampler0, ShaderType_Pixel);
        UseMaterial(GetAsset(ent->material));
        DrawMesh(GetAsset(ent->mesh));
    }
    
    R_ImGuiDrawFrame();
    
    R_PresentFrame();
}
