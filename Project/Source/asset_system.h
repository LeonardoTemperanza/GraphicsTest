
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

struct PipelineHandle
{
    u32 idx;
    operator R_Pipeline();
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
    Array<R_Mesh>     meshes;
    Array<R_Texture>  textures;
    Array<R_Shader>   shaders;
    Array<R_Pipeline> pipelines;
    Array<Material>   materials;
    
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
void LoadPipeline(R_Pipeline* pipeline, String path);
void LoadMaterial(Material* material, String path);

void LoadMesh(const char* path);
void LoadTexture(const char* path);
void LoadPipeline(const char* path);
void LoadMaterial(const char* path);

MeshHandle     GetMeshByPath(const char* path);
TextureHandle  GetTextureByPath(const char* path);
MaterialHandle GetMaterialByPath(const char* path);

// Scene serialization
struct EntityManager;
void SerializeScene(EntityManager* man, const char* path);
void UnloadScene(EntityManager* man);
void LoadScene(EntityManager* man, const char* path);
