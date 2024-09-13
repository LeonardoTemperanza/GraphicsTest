
#pragma once

#include "base.h"

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

struct Material
{
    const char* vertShaderPath;
    const char* pixelShaderPath;
    Slice<R_UniformValue> uniforms;
    Slice<AssetKey> textures;
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
R_Cubemap GetCubemapByPath(String path);
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
void UseMaterial(AssetKey material);

// Text file handling

// NOTE: @cleanup Duplicated code from metaprogram. Unify later
// ...Except that the tokenizer is slightly different... For example
// comments in C are // and /**/, while in our text files it's #.
// So what to do? In my opinion it's better to keep them separate
// for now, a little duplication is not going to kill anyone

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
    Tok_Slash,
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

struct Parser
{
    String path;
    
    Token* at;
    bool foundError;
};

Token GetNextToken(Tokenizer* t);
Slice<Token> GetTokens(char* contents, Arena* dst);
void EatAllWhitespace(Tokenizer* t);
bool IsWhitespace(char c);
bool IsAlpha(char c);
bool IsNumber(char c);
void ParseError(Parser* p, Token* token, const char* message);
void EatRequiredToken(Parser* p, TokenKind kind);
