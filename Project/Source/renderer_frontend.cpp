
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
    R_Sampler bilinear;  // Linear sampler used for most things
    
    // Render passes
    R_Framebuffer mainFramebuffer;  // Used to render the main scene with hdr and multisample
    R_Framebuffer postProcess;      // Used for post processing. It's still hdr, but not multisampled
    
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
    
    res.bilinear = R_SamplerAlloc();
    
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
    
    R_Texture2D mainFramebufferColor = R_Texture2DAlloc(TextureFormat_RGBA_SRGB,
                                                        w, h, nullptr,
                                                        TextureUsage_ShaderResource | TextureUsage_Drawable,
                                                        TextureMutability_Mutable, false, 4);
    R_Texture2D mainFramebufferDepth = R_Texture2DAlloc(TextureFormat_DepthStencil,
                                                        w, h, nullptr,
                                                        0, 
                                                        TextureMutability_Mutable, false, 4);
    res.mainFramebuffer = R_FramebufferAlloc(w, h, &mainFramebufferColor, 1, mainFramebufferDepth);
    
    R_Texture2D postProcessColor = R_Texture2DAlloc(TextureFormat_RGBA_HDR,
                                                    w, h, nullptr,
                                                    TextureUsage_ShaderResource | TextureUsage_Drawable,
                                                    TextureMutability_Mutable, false);
    R_Texture2D postProcessDepth = R_Texture2DAlloc(TextureFormat_DepthStencil,
                                                    w, h, nullptr,
                                                    0, 
                                                    TextureMutability_Mutable, false);
    res.postProcess = R_FramebufferAlloc(w, h, &postProcessColor, 1, postProcessDepth);
    
    const R_Framebuffer* screen = R_GetScreen();
    //R_FramebufferBind(screen);
    R_FramebufferBind(&res.mainFramebuffer);
}

void UpdateFramebuffers()
{
    auto& res = renderResources;
    
    s32 w, h;
    OS_GetClientAreaSize(&w, &h);
    
    R_FramebufferResize(&res.mainFramebuffer, w, h);
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
    
    R_FramebufferClear(&res.mainFramebuffer, BufferMask_Depth | BufferMask_Stencil);
    R_FramebufferFillColor(&res.mainFramebuffer, 0, 0.5f, 0.5f, 0.5f, 1.0f);
    
    R_FramebufferBind(&res.mainFramebuffer);
    
    R_ShaderBind(GetAsset(res.staticVertShader));
    
    
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
        
        R_SamplerBind(&res.bilinear, CodeSampler0, ShaderType_Pixel);
        UseMaterial(GetAsset(ent->material));
        DrawMesh(GetAsset(ent->mesh));
    }
    
    R_FramebufferResolve(&res.mainFramebuffer, R_GetScreen());
    
    R_FramebufferBind(R_GetScreen());
    R_ImGuiDrawFrame();
    
    R_PresentFrame();
}
