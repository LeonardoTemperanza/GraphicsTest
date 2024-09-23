
#pragma once

#include "base.h"

#include "renderer/renderer_generic.h"
#include "serialization.h"
#include "metaprogram_custom_keywords.h"

struct PipelinePath
{
    String vertPath;
    String pixelPath;
};

struct MeshHandle
{
    u32 idx;
    operator R_Mesh();
};

struct TextureHandle
{
    u32 idx;
    operator R_Texture();
};

struct ShaderHandle
{
    u32 idx;
    operator R_Shader();
};

struct Pipeline;
struct PipelineHandle
{
    u32 idx;
    operator Pipeline();
};

struct Material;
struct MaterialHandle
{
    u32 idx;
    operator Material();
};

struct AssetSystem
{
    // NOTE: Index 0 of each asset array
    // is reserved for the default asset
    // of that type. This means that a
    // handle of 0 is always valid
    // This contains all resources which
    // need to be looked up, and which have
    // an individual lifetime.
    Array<R_Mesh>    meshes;
    Array<R_Texture> textures;
    Array<R_Shader>  shaders;
    Array<Pipeline>  pipelines;
    Array<Material>  materials;
    
    // Acceleration structures to speed up
    // some of the queries
    StringMap<MeshHandle>     pathToMesh;
    StringMap<TextureHandle>  pathToTexture;
    StringMap<ShaderHandle>   pathToShader;
    StringMap<MaterialHandle> pathToMaterial;
};

struct Material
{
    PipelineHandle pipeline;
    
    Array<R_UniformValue> uniforms;
    Array<TextureHandle> textures;
};

struct Pipeline
{
    ShaderHandle vert;
    ShaderHandle pixel;
    R_Pipeline obj;
};

void InitAssetSystem();
void ReserveSlotForDefaultAssets();
template<typename t>
t DefaultAssetHandle();
ShaderHandle DefaultVertexShaderHandle();
ShaderHandle DefaultPixelShaderHandle();

// Utility functions for new types
void UseMaterial(Material mat);

// Hot reloading. To be performed once per frame or once per few frames
void HotReloadAssets();

void LoadMesh(R_Mesh* mesh, String path);
void LoadTexture(R_Texture* texture, String path);
void LoadShader(R_Shader* shader, String path, ShaderKind kind);
void LoadPipeline(R_Pipeline* pipeline, String path);
void LoadMaterial(Material* material, String path);

void LoadMesh(const char* path);
void LoadTexture(const char* path);
void LoadShader(const char* path, ShaderKind kind);
void LoadPipeline(const char* path);
void LoadMaterial(const char* path);

MeshHandle     GetMeshByPath(const char* path);
TextureHandle  GetTextureByPath(const char* path);
ShaderHandle   GetShaderByPath(const char* path, ShaderKind kind);
MaterialHandle GetMaterialByPath(const char* path);

PipelineHandle GetPipelineByPath(const char* vert, const char* pixel);
PipelineHandle GetPipelineByHandles(ShaderHandle vert, ShaderHandle pixel);

// Scene serialization
struct EntityManager;
void SerializeScene(EntityManager* man, const char* path);
void UnloadScene(EntityManager* man);
void LoadScene(EntityManager* man, const char* path);
