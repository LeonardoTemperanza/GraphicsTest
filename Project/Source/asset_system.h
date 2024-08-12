
// A few notes so i don't forget things
// There are string ids which signify intent for a particular asset.
// To reference assets in files those will primarily be used instead of the path,
// so that the path itself can be changed easily. There are "assetbinding" files that
// specify the file path for each string id (or asset intent).
// Models are just binary files with vertex info and others
// Shaders are just binary files with shader binaries (or sources) for the corresponding graphics apis
// Textures are just png files (the simplest files here)
// Materials are the only textual files at the moment (other than asset bindings) which contain references to string ids

// A string map can be used for looking up resources from their string id. Also used for hot reloading.
// Textual files like materials which reference other assets via their string id, will be loaded using the string map, to prevent
// loading the same texture/shader multiple times, wasting space and time.

#pragma once

#include "base.h"
//#include "renderer/renderer_generic.h"

#include "serialization.h"

struct PipelinePath
{
    const char* vertPath;
    const char* pixelPath;
};

struct CubemapPath
{
    const char* topPath;
    const char* bottomPath;
    const char* leftPath;
    const char* rightPath;
    const char* frontPath;
    const char* backPath;
};

struct AssetSystem
{
    // @tmp TODO: This will soon get restructured anyway, but this will let
    // us conveniently load things for now
    Array<const char*>  modelPaths;
    Array<Model*>       models;
    Array<const char*>  texturePaths;
    Array<R_Texture>    textures;
    Array<const char*>  shaderPaths;
    Array<R_Shader>     shaders;
    Array<PipelinePath> pipelinePaths;
    Array<R_Pipeline>   pipelines;
    Array<CubemapPath>  cubemapPaths;
    Array<R_Cubemap>    cubemaps;
};

//////////////// end of c++ nightmare



struct Mesh
{
    bool hasTextureCoords;
    bool hasBones;
    
    Slice<Vertex> verts;
    Slice<s32>    indices;
    
    R_Buffer handle;
};

struct Model
{
    String id;
    Slice<Mesh> meshes;
    R_Pipeline pipeline;
};

// Based on development/release, this could
// have a string in it or an int and stuff
struct AssetKey
{
    int handle;
    const char* path;  // Will later be a string id
};

void LoadScene(const char* path);
Model* LoadModel(const char* path);
R_Texture LoadTexture(const char* path);
R_Shader LoadShader(const char* path);
void LoadAssetBinding(const char* path);

void UnloadScene(const char* path);

// TODO: everything is lazily loaded for now,
// will change later

Model* GetModel(const char* path);
R_Texture GetTexture(const char* path);
R_Shader GetShader(const char* path);
R_Cubemap GetCubemap(const char* topPath, const char* bottomPath, const char* leftPath, const char* rightPath, const char* frontPath, const char* backPath);
R_Pipeline GetPipeline(const char* vertShaderPath, const char* pixelShaderPath);

// Some utility functions that lets us use the assets (stuff like DrawModel, PlaySound, etc.)
