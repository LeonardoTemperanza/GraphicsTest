
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

static R_VertLayout staticLayout;
static R_VertLayout skinnedLayout;

static R_Sampler bilinear;

static R_Framebuffer mainFramebuffer;
static R_Framebuffer postProcess;

static R_Buffer perView;
static R_Buffer perObj;

static VertShaderHandle staticVertShader;

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
        staticLayout = R_VertLayoutAlloc(attribs, ArrayCount(attribs));
    }
    
    {
        R_VertAttrib attribs[] =
        {
            { .type=VertAttrib_Pos, .bufferSlot=0, .offset=offsetof(Vertex, pos), },
            { .type=VertAttrib_Normal, .bufferSlot=0, .offset=offsetof(Vertex, normal), },
            { .type=VertAttrib_TexCoord, .bufferSlot=0, .offset=offsetof(Vertex, texCoord), },
            { .type=VertAttrib_Tangent, .bufferSlot=0, .offset=offsetof(Vertex, tangent), }
        };
        skinnedLayout = R_VertLayoutAlloc(attribs, ArrayCount(attribs));
    }
    
    bilinear = R_SamplerAlloc();
    
    staticVertShader = AcquireVertShader("CompiledShaders/model2proj.shader");
    
    perView = R_BufferAlloc(BufferFlag_Dynamic | BufferFlag_ConstantBuffer, sizeof(PerView), sizeof(PerView), nullptr);
    perObj  = R_BufferAlloc(BufferFlag_Dynamic | BufferFlag_ConstantBuffer, sizeof(PerObj), sizeof(PerObj), nullptr);
    
    R_BufferUniformBind(&perObj,  PerObjSlot,  ShaderType_Vertex);
    R_BufferUniformBind(&perView, PerViewSlot, ShaderType_Vertex);
    
    R_Texture2D mainFramebufferColor = R_Texture2DAlloc(TextureFormat_RGBA_SRGB,
                                                        w, h, nullptr,
                                                        TextureUsage_ShaderResource | TextureUsage_Drawable,
                                                        TextureMutability_Mutable, false, 4);
    R_Texture2D mainFramebufferDepth = R_Texture2DAlloc(TextureFormat_DepthStencil,
                                                        w, h, nullptr,
                                                        0, 
                                                        TextureMutability_Mutable, false, 4);
    mainFramebuffer = R_FramebufferAlloc(w, h, &mainFramebufferColor, 1, mainFramebufferDepth);
    
    R_Texture2D postProcessColor = R_Texture2DAlloc(TextureFormat_RGBA_HDR,
                                                    w, h, nullptr,
                                                    TextureUsage_ShaderResource | TextureUsage_Drawable,
                                                    TextureMutability_Mutable, false);
    R_Texture2D postProcessDepth = R_Texture2DAlloc(TextureFormat_DepthStencil,
                                                    w, h, nullptr,
                                                    0, 
                                                    TextureMutability_Mutable, false);
    postProcess = R_FramebufferAlloc(w, h, &postProcessColor, 1, postProcessDepth);
    
    const R_Framebuffer* screen = R_GetScreen();
    //R_FramebufferBind(screen);
    R_FramebufferBind(&mainFramebuffer);
}

void UpdateFramebuffers()
{
    s32 w, h;
    OS_GetClientAreaSize(&w, &h);
    
    R_FramebufferResize(&mainFramebuffer, w, h);
}

void RenderResourcesCleanup()
{
    // Not a priority right now. These resources are supposed to
    // live as long as the program does, and the os will clean these
    // up for you, so right now we don't worry too much.
    
#if 0
    auto& res = renderResources;
    
    R_VertLayoutFree(&staticLayout);
    R_VertLayoutFree(&skinnedLayout);
    
    R_SamplerFree(&commonSampler);
    
    R_Texture2DFree(&selectionColor);
    R_Texture2DFree(&selectionDepth);
    R_FramebufferFree(&selectionBuffer);
    
    R_Texture2DFree(&outlinesColor);
    R_Texture2DFree(&outlinesDepth);
    R_FramebufferFree(&outlines);
    
    //ReleaseCubemap(skybox);
#endif
}

void RenderFrame(EntityManager* entities, CamParams cam)
{
    R_FramebufferClear(&mainFramebuffer, BufferMask_Depth | BufferMask_Stencil);
    R_FramebufferFillColor(&mainFramebuffer, 0, 0.5f, 0.5f, 0.5f, 1.0f);
    
    R_FramebufferBind(&mainFramebuffer);
    
    R_ShaderBind(GetAsset(staticVertShader));
    
    
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
    
    R_VertLayoutBind(&staticLayout);
    
    // Draw entities
    R_BufferUniformBind(&perView, PerViewSlot, ShaderType_Vertex);
    
    s32 w, h;
    OS_GetClientAreaSize(&w, &h);
    
    auto view2Proj = View2ProjPerspectiveMatrix(cam.nearClip, cam.farClip, cam.fov, (float)w, (float)h);
    
    {
        PerView data = {};
        data.world2View = World2ViewMatrix(cam.pos, cam.rot);
        data.view2Proj  = R_ConvertClipSpace(view2Proj);
        R_BufferUpdateStruct(&perView, data);
    }
    for_live_entities(entities, ent)
    {
        if(ent->flags & EntityFlags_NoMesh) continue;
        
        {
            PerObj data = {};
            data.model2World = ComputeWorldTransform(entities, ent);
            data.normalMat = transpose(ComputeTransformInverse(data.model2World));
            R_BufferUpdateStruct(&perObj, data);
        }
        
        R_SamplerBind(&bilinear, CodeSampler0, ShaderType_Pixel);
        UseMaterial(GetAsset(ent->material));
        DrawMesh(GetAsset(ent->mesh));
    }
    
    R_FramebufferResolve(&mainFramebuffer, R_GetScreen());
    
    R_FramebufferBind(R_GetScreen());
    R_ImGuiDrawFrame();
    
    R_PresentFrame();
}
