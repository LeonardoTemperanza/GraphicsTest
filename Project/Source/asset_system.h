
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

enum AssetKind
{
    Asset_None = 0,
    Asset_Mesh,
    Asset_Texture,
    Asset_Shader,
    Asset_Material,
    
    Asset_Count
};

struct AssetMetadata
{
    AssetKind kind;
    String path;
};

struct AssetSystem
{
    // NOTE: Index 0 of each asset array
    // is reserved for the default asset
    // of that type. This means that a
    // handle of 0 is always valid
    
    // Assets
    Array<R_Mesh>    meshes;
    Array<R_Texture> textures;
    Array<R_Shader>  shaders;
    Array<Material>  materials;
    
    // Resources
    Array<Pipeline> pipelines;
    // array of samplers as well?
    
    // Acceleration structures to speed up
    // some of the queries
    StringMap<MeshHandle>     pathToMesh;
    StringMap<TextureHandle>  pathToTexture;
    StringMap<TextureHandle>  pathToCubemap;
    StringMap<ShaderHandle>   pathToShader;
    StringMap<MaterialHandle> pathToMaterial;
    
    // Metadata
    Array<AssetMetadata> metadata[Asset_Count];
};

struct Material
{
    ShaderHandle pixelShader;
    
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
void LoadShader(R_Shader* shader, String path, ShaderKind kind);
void LoadPipeline(R_Pipeline* pipeline, String path);
void LoadMaterial(Material* material, String path);
void LoadCubemap(R_Texture* cubemap, String path);

void LoadMesh(const char* path);
void LoadTexture(const char* path);
void LoadShader(const char* path, ShaderKind kind);
void LoadPipeline(const char* path);
void LoadMaterial(const char* path);

// Asset lookup by path

// @cleanup These functions should be named "Search..." instead of "Get..." (or maybe "LazyLoad..."),
// because the former better communicates the fact that it's a potentially expensive operation
MeshHandle     GetMeshByPath(const char* path);
TextureHandle  GetTextureByPath(const char* path);
ShaderHandle   GetShaderByPath(const char* path, ShaderKind kind);
MaterialHandle GetMaterialByPath(const char* path);
// TODO: Hey! Missing cubemap function here!

// Common resource lookup

AssetMetadata GetMetadata(u32 handle, AssetKind kind);

// Scene serialization
struct EntityManager;
void SerializeScene(EntityManager* man, const char* path);
void UnloadScene(EntityManager* man);
void LoadScene(EntityManager* man, const char* path);
