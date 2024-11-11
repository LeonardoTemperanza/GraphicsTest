
#pragma once

#include "base.h"

#include "renderer_backend/generic.h"
#include "renderer_frontend.h"
#include "serialization.h"
#include "metaprogram_custom_keywords.h"

enum AssetKind
{
    Asset_Mesh = 0,
    Asset_VertShader,
    Asset_PixelShader,
    //Asset_Material,
    //Asset_Texture2D,
    //Asset_Cubemap,
    
    Asset_Count
};

struct AssetHandle { u32 slot; };

// These are type-safe typedefs
struct MeshHandle        : public AssetHandle {};
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
        Mesh mesh;
        R_Shader shader;
        //R_Texture2D texture2D;
        //R_Cubemap cubemap;
    };
    
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

void AssetSystemInit();
void AssetSystemSetMode(AssetSystemMode mode);

// Templatizing it is impossible (or very convoluted), trust me
Mesh*        GetAsset(MeshHandle handle);
R_Shader*    GetAsset(VertShaderHandle handle);
R_Shader*    GetAsset(PixelShaderHandle handle);
Material     GetAsset(MaterialHandle handle);
R_Texture2D  GetAsset(Texture2DHandle handle);
R_Cubemap    GetAsset(CubemapHandle handle);

MeshHandle AcquireMesh(String path);
VertShaderHandle AcquireVertShader(String path);
PixelShaderHandle AcquirePixelShader(String path);
MaterialHandle AcquireMaterial(String path);
Texture2DHandle AcquireTexture2D(String path);
CubemapHandle AcquireCubemap(String path);

MeshHandle AcquireMesh(const char* path);
VertShaderHandle AcquireVertShader(const char* path);
PixelShaderHandle AcquirePixelShader(const char* path);
MaterialHandle AcquireMaterial(const char* path);
Texture2DHandle AcquireTexture2D(const char* path);
CubemapHandle AcquireCubemap(const char* path);

//void ReleaseModel(ModelHandle handle);
void ReleaseVertShader(VertShaderHandle handle);
void ReleasePixelShader(PixelShaderHandle handle);
void ReleaseMaterial(MaterialHandle handle);
void ReleaseTexture2D(Texture2DHandle handle);
void ReleaseCubemap(CubemapHandle handle);

// Hot reloading. To be performed once per frame or once per few frames
void HotReloadAssets(Arena* frameArena);

// Asset loading functions
bool LoadMesh(Asset* mesh, String path);
void LoadTexture(Asset* texture, String path);
bool LoadShader(Asset* shader, String path, R_ShaderType type);
void LoadPipeline(Asset* pipeline, String path);
void LoadMaterial(Asset* material, String path);
void LoadCubemap(Asset* cubemap, String path);
