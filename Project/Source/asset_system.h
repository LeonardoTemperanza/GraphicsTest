
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

typedef u64 AssetId;

// Depending on development or release, this can 
struct AssetKey
{
#ifdef Development
    const char* path;
#else
    u32 idx;
#endif
};

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

struct Material
{
    R_Pipeline* pipeline;
    Slice<R_UniformValue> uniforms;
    Slice<AssetKey> textures;
};

void LoadScene(const char* path);
Model* LoadModel(const char* path);
R_Texture LoadTexture(const char* path);
R_Shader LoadShader(const char* path);
void LoadAssetBinding(const char* path);

void UnloadScene(const char* path);

// TODO: everything is lazily loaded for now,
// will change later

Model* GetModelByPath(const char* path);
R_Texture GetTextureByPath(const char* path);
R_Shader GetShaderByPath(const char* path);
// Make it so the user only supplies one path, and then the rest is derived
// (by adding _top, _bottom, _left, _right to the path
R_Cubemap GetCubemapByPath(const char* topPath, const char* bottomPath, const char* leftPath, const char* rightPath, const char* frontPath, const char* backPath);

R_Pipeline GetPipelineByPath(const char* vert, const char* pixel);

Model* GetModelById(AssetId id);
R_Texture GetTextureById(AssetId id);
R_Shader GetShaderById(AssetId id);
R_Pipeline GetPipelineById(AssetId vert, AssetId pixel);

Model* GetModelByTag(const char* tag);
R_Texture GetTextureByTag(const char* tag);
R_Shader GetShaderByTag(const char* tag);
R_Shader GetPipelineByTag(const char* vert, const char* pixel);

// Utility functions
void UseMaterial(AssetKey material)
{
    
}

// Text file handling
enum TokenKind
{
    TokKind_None = 0,
    TokKind_Error,
    TokKind_Ident,
    TokKind_Colon,
    TokKind_Comma,
    TokKind_Dot,
    TokKind_OpenCurly,
    TokKind_CloseCurly,
    TokKind_FloatConst,
    TokKind_IntConst,
    TokKind_EOF,
    
    // Keywords
    TokKind_True,
    TokKind_False,
    TokKind_MaterialName,
    TokKind_VertexShader,
    TokKind_PixelShader,
    TokKind_Values
};

struct Token
{
    TokenKind kind;
    String text;
    
    int lineNum;
    
    union
    {
        double doubleVal;
        s64    intVal;
    };
};

struct Tokenizer
{
    char* start;
    char* at;
    int numLines;
    char* lineStart;
    
    bool error;
};

inline bool IsStartIdent(char c);
inline bool IsNumeric(char c);
inline bool IsMiddleIdent(char c);
int EatAllWhitespace(char** at);
Token NextToken(Tokenizer* tokenizer);

// TODO: Some utility functions that lets us use the assets (stuff like DrawModel, PlaySound, etc.)
