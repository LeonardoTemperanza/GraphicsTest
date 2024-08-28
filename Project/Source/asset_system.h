
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

struct Material;

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
    Array<const char*>  materialPaths;
    Array<Material*>    materials;
};

typedef u64 AssetId;

// NOTE: a "" path or a uint_max idx means a null asset
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
    Slice<Mesh> meshes;
};

struct Material
{
    AssetKey pipeline;
    Slice<R_UniformValue> uniforms;
    Slice<AssetKey> textures;
};

AssetKey MakeAssetKey(const char* path);

Model* LoadModel(const char* path);
R_Texture LoadTexture(const char* path);
R_Shader LoadShader(const char* path, ShaderKind kind);
Material* LoadMaterial(const char* path);

// TODO: everything is lazily loaded for now,
// will change later

Model* GetModelByPath(const char* path);
R_Texture GetTextureByPath(const char* path);
R_Shader GetShaderByPath(const char* path, ShaderKind kind);
// Make it so the user only supplies one path, and then the rest is derived
// (by adding _top, _bottom, _left, _right to the path
R_Cubemap GetCubemapByPath(const char* topPath, const char* bottomPath, const char* leftPath, const char* rightPath, const char* frontPath, const char* backPath);

R_Pipeline GetPipelineByPath(const char* vert, const char* pixel);
Material* GetMaterialByPath(const char* path);

Model* GetModel(AssetKey key);
R_Texture GetTexture(AssetKey key);
Material* GetMaterial(AssetKey key);

// Utility functions
void UseMaterial(AssetKey material);

// Text file handling

// NOTE: @cleanup Duplicated code from metaprogram. Unify later

enum TokenKind
{
    Tok_OpenParen,
    Tok_CloseParen,
    Tok_OpenBracket,
    Tok_CloseBracket,
    Tok_OpenBrace,
    Tok_CloseBrace,
    Tok_Colon,
    Tok_Semicolon,
    Tok_Asterisk,
    Tok_String,
    Tok_Ident,
    Tok_Unknown,
    
    Tok_EndOfStream
};

struct Token
{
    TokenKind kind;
    String text;
    
    int lineNum;
    
    // Relative to the start of the line
    int startPos;
    
    static const Token null;
};

struct Tokenizer
{
    char* start;
    char* at;
    int lineNum;
    char* lineStart;
};

Token GetNextToken(Tokenizer* t);
void EatAllWhitespace(Tokenizer* t);
bool IsWhitespace(char c);
bool IsAlpha(char c);
bool IsNumber(char c);
