
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
    Asset_Material,
    Asset_Texture2D,
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

// It's reasonable to define data types that simply
// group asset handles together here.
struct Material
{
    PixelShaderHandle shader;
    Array<Texture2DHandle> textures;
};

struct Asset
{
    u32 slot;  // For debugging
    AssetKind kind;
    // The content is set to the default asset if loading was unsuccessful
    union
    {
        Mesh mesh;
        R_Shader shader;
        Material material;
        R_Texture2D texture2D;
        //R_Cubemap cubemap;
    };
    
    bool isLoaded;  // False if using a default asset because of a loading error
};

struct AssetSystem
{
    Array<Asset> assets;
    Array<u32>   freeSlots;
    Asset defaultAssets[Asset_Count];
    
    struct MapValue { AssetKind kind; u32 slot; };
    StringMap<MapValue> pathMapping;
};

void AssetSystemInit();

// Templatizing it is impossible (or very convoluted), trust me
Mesh*        GetAsset(MeshHandle handle);
R_Shader*    GetAsset(VertShaderHandle handle);
R_Shader*    GetAsset(PixelShaderHandle handle);
Material*    GetAsset(MaterialHandle handle);
R_Texture2D* GetAsset(Texture2DHandle handle);
R_Cubemap*   GetAsset(CubemapHandle handle);

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

// Hot reloading. To be performed once per frame or once per few frames
void HotReloadAssets(Arena* frameArena);

// Asset loading functions
Mesh        LoadMesh(String path, bool* ok);
R_Texture2D LoadTexture2D(String path, bool* ok);
R_Shader    LoadShader(String path, ShaderType type, bool* ok);
Material    LoadMaterial(String path, bool* ok);
R_Cubemap   LoadCubemap(String path, bool* ok);

