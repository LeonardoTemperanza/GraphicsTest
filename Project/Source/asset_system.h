
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

struct Material;

struct AssetSystem
{
    // @tmp TODO: This will soon get restructured anyway, but this will let
    // us conveniently load things for now
    Array<String>       meshPaths;
    Array<R_Mesh>       meshes;
    Array<String>       texturePaths;
    Array<R_Texture>    textures;
    Array<String>       shaderPaths;
    Array<R_Shader>     shaders;
    Array<PipelinePath> pipelinePaths;
    Array<R_Pipeline>   pipelines;
    Array<String>       cubemapPaths;
    Array<R_Cubemap>    cubemaps;
    Array<String>       materialPaths;
    Array<Material>     materials;
};

typedef u64 AssetId;

// NOTE: a "" path or a uint_max idx means a null asset
#ifdef Development
introspect()
struct AssetKey
{
    DynString path;
    int a;
};
#else
struct AssetKey
{
    u32 idx;
};
#endif

// NOTE: All this stuff needs to get freed. We'll deal with that later
// @leak
struct Material
{
    const char* vertShaderPath;
    const char* pixelShaderPath;
    
    Array<R_UniformValue> uniforms;
    Array<String> textures;
};

AssetKey MakeAssetKey(const char* path);
AssetKey MakeNullAssetKey();
void FreeAssetKey(AssetKey* key);

R_Mesh    LoadMesh(String path, bool* outSuccess);
R_Texture LoadTexture(String path, bool* outSuccess);
R_Shader  LoadShader(String path, ShaderKind kind, bool* outSuccess);
Material  LoadMaterial(String path, bool* outSuccess);
Material  DefaultMaterial();

// TODO: everything is lazily loaded for now,
// will change later

R_Mesh    GetMeshByPath(String path);
R_Texture GetTextureByPath(String path);
R_Shader  GetShaderByPath(String path, ShaderKind kind);
// NOTE: _top, _bottom, _left, _right, _front and _back is added (before the extension)
// to load each individual texture
R_Cubemap  GetCubemapByPath(String path);
R_Pipeline GetPipelineByPath(String vert, String pixel);
Material   GetMaterialByPath(String path);

// Utilities for C strings
R_Mesh    GetMeshByPath(const char* path);
R_Texture GetTextureByPath(const char* path);
R_Shader  GetShaderByPath(const char* path, ShaderKind kind);
R_Cubemap GetCubemapByPath(const char* path);
R_Pipeline GetPipelineByPath(const char* vert, const char* pixel);
Material   GetMaterialByPath(const char* path);

R_Mesh     GetMesh(AssetKey key);
R_Pipeline GetPipeline(AssetKey vert, AssetKey pixel);
R_Texture  GetTexture(AssetKey key);
Material   GetMaterial(AssetKey key);

// Utility functions
// Check if the material specifies the correct uniforms and textures, etc.
// Logs the exact problem to the console
bool CheckMaterial(Material mat, R_Pipeline pipeline);
void UseMaterial(AssetKey material);
