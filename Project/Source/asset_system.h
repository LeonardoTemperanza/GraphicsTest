
#pragma once

#include "base.h"

#include "renderer_backend/generic.h"
#include "serialization.h"
#include "metaprogram_custom_keywords.h"

#if 1

struct PipelinePath
{
    String vertPath;
    String pixelPath;
};

struct MeshHandle
{
    u32 idx;
    //operator R_Mesh();
};

struct TextureHandle
{
    u32 idx;
    //operator R_Texture();
};

struct ShaderHandle
{
    u32 idx;
    //operator R_Shader();
};

struct Pipeline;
struct PipelineHandle
{
    u32 idx;
    //operator Pipeline();
};

struct Material;
struct MaterialHandle
{
    u32 idx;
    //operator Material();
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

struct R_Mesh {};
typedef R_Texture2D R_Texture;

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
    
    // Acceleration structures to speed up
    // some of the queries. TODO: Could put a ref count here for editor stuff
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
    int a;
    
#if 0
    ShaderHandle pixelShader;
    
    Array<R_UniformValue> uniforms;
    Array<TextureHandle> textures;
#endif
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
void HotReloadAssets(Arena* frameArena);

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


#else

enum AssetKind
{
    Asset_Model = 0,
    Asset_VertShader,
    Asset_PixelShader,
    Asset_Material,
    Asset_Texture2D,
    Asset_Cubemap,
    
    Asset_Count
};

struct AssetHandle { u32 slot; };

// These are type-safe typedefs
struct ModelHandle       : public AssetHandle {};
struct VertShaderHandle  : public AssetHandle {};
struct PixelShaderHandle : public AssetHandle {};
struct MaterialHandle    : public AssetHandle {};
struct Texture2DHandle   : public AssetHandle {};
struct CubemapHandle     : public AssetHandle {};

struct Asset
{
    AssetKind kind;
    // The content is set to the default asset if loading was unsuccessful
    union
    {
        Model model;
        R_Shader shader;
        R_Texture2D texture2D;
        R_Cubemap cubemap;
    } content;
    
    String path;
    bool isLoaded;
    u64 refCount;
};

enum AssetSystemMode
{
    AssetSystem_Editor,
    AssetSystem_Runtime  // Does not actually free the resources until we change scenes
};

struct AssetSystem
{
    AssetSystemMode mode;
    
    Array<Asset> assets;
    Array<u32>   freeSlots;
    Asset defaultAssets[Asset_Count];
    
    struct MapValue { AssetKind kind; u32 slot; };
    StringMap<MapValue> pathMapping;
};

struct Material
{
    
};

void AssetSystemSetMode(AssetSystemMode mode);

// Templatizing it is impossible (or very convoluted), trust me
Model       GetAsset(ModelHandle handle);
R_Shader    GetAsset(VertShaderHandle handle);
R_Shader    GetAsset(PixelShaderHandle handle);
Material    GetAsset(MaterialHandle handle);
R_Texture2D GetAsset(Texture2DHandle handle);
R_Cubemap   GetAsset(CubemapHandle handle);

ModelHandle AcquireModel(String path);
void ReleaseModel(ModelHandle* handle);
VertShaderHandle AcquireVertShader(String path);
void ReleaseVertShader(VertShaderHandle* handle);
PixelShaderHandle AcquirePixelShader(String path);
void ReleasePixelShader(PixelShaderHandle* handle);
MaterialHandle AcquireMaterial(String path);
void ReleaseMaterial(MaterialHandle* handle);
Texture2DHandle AcquireTexture2D(String path);
void ReleaseTexture2D(Texture2DHandle* handle);
CubemapHandle AcquireCubemap(String path);
void ReleaseCubemap(CubemapHandle* handle);

#endif
