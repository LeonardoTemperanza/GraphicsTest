
#include "asset_system.h"
#include "parser.h"

static Arena sceneArena = ArenaVirtualMemInit(GB(4), MB(2));

static AssetSystem assetSystem;

AssetKey MakeAssetKey(const char* path)
{
    AssetKey key = {0};
    key.path = path;
    return key;
}

Model* LoadModel(const char* path)
{
    ScratchArena scratch;
    
    Model* res = ArenaZAllocTyped(Model, &sceneArena);
    
    bool success = true;
    String contents = LoadEntireFile(path, scratch, &success);
    if(!success)
    {
        Log("Failed to load file '%s'", path);
        
        // res->meshes.len is already 0, so it's
        // already sufficient for a fallback model
        return res;
    }
    
    char** cursor;
    char* c = (char*)contents.ptr;
    cursor = &c;
    
    String magicBytes = Next(cursor, sizeof("model")-1);  // Excluding null terminator
    if(magicBytes != "model")
    {
        Log("Attempted to load file '%s' as a model, which it is not.", path);
        
        // res->meshes.len is already 0, so it's
        // already sufficient for a fallback model
        return res;
    }
    
    s32 version      = Next<s32>(cursor);
    s32 numMeshes    = Next<s32>(cursor);
    s32 numMaterials = Next<s32>(cursor);
    res->meshes.ptr   = ArenaZAllocArray(Mesh, numMeshes, &sceneArena);
    res->meshes.len   = numMeshes;
    
    for(int i = 0; i < numMeshes; ++i)
    {
        auto& mesh = res->meshes[i];
        
        s32 numVerts    = Next<s32>(cursor);
        s32 numIndices  = Next<s32>(cursor);
        s32 materialIdx = Next<s32>(cursor);
        bool hasTextureCoords = Next<bool>(cursor);
        
        Slice<Vertex> verts = Next<Vertex>(cursor, numVerts);
        Slice<s32> indices  = Next<s32>(cursor, numIndices);
        
        mesh.verts   = ArenaPushSlice(&sceneArena, verts);
        mesh.indices = ArenaPushSlice(&sceneArena, indices);
        mesh.handle = R_UploadMesh(mesh.verts, mesh.indices);
    }
    
    return res;
}

R_Texture LoadTexture(const char* path)
{
    String stbImage = {0};
    int width, height, numChannels;
    stbImage.ptr = (char*)stbi_load(path, &width, &height, &numChannels, 0);
    stbImage.len = width * height * numChannels;
    if(!stbImage.ptr)
    {
        // Fallback
        // TODO: Fallback texture
        TODO;
    }
    
    R_Texture res = R_UploadTexture(stbImage, width, height, numChannels);
    stbi_image_free((void*)stbImage.ptr);
    return res;
}

R_Shader LoadShader(const char* path, ShaderKind kind)
{
    ScratchArena scratch;
    
    bool success = true;
    String contents = LoadEntireFile(path, scratch, &success);
    if(!success)
    {
        Log("Failed to load file '%s'\n", path);
        return R_DefaultShader(kind);
    }
    
    char** cursor;
    char* c = (char*)contents.ptr;
    cursor = &c;
    
    String magicBytes = Next(cursor, sizeof("shader")-1);  // Excluding null terminator
    if(magicBytes != "shader")
    {
        Log("Attempted to load file '%s' as a shader, which it is not.", path);
        return R_DefaultShader(kind);
    }
    
    u32 version = Next<u32>(cursor);
    if(version != 0)
    {
        Log("Attempted to load file '%s' as a shader, but its version is unsupported.", path);
        return R_DefaultShader(kind);
    }
    
    char* headerPtr = *cursor;
    ShaderBinaryHeader_v0 header = Next<ShaderBinaryHeader_v0>(cursor);
    if(header.shaderKind != kind)
    {
        Log("Attempted to load wrong type of shader '%s'", path);
        return R_DefaultShader(kind);
    }
    
    String dxil        = {.ptr=headerPtr+header.dxil, .len=header.dxilSize};
    String vulkanSpirv = {.ptr=headerPtr+header.vulkanSpirv, .len=header.vulkanSpirvSize};
    String glsl        = {.ptr=headerPtr+header.glsl, .len=header.glslSize};
    return R_CompileShader((ShaderKind)header.shaderKind, dxil, vulkanSpirv, glsl);
}

Material* LoadMaterial(const char* path)
{
    static Material tmp;  // @tmp instead of returning nullptr which will crash
    return &tmp;
    
    Material mat = {0};
    ScratchArena scratch;
    
    bool success = true;
    String contents = LoadEntireFile(path, scratch, &success);
    if(!success)
    {
        Log("Failed to load file '%s'\n", path);
        //return mat;
    }
    
    return nullptr;
}

void UnloadScene()
{
    ArenaFreeAll(&sceneArena);
}

Model* GetModelByPath(const char* path)
{
    int foundIdx = -1;
    for(int i = 0; i < assetSystem.modelPaths.len; ++i)
    {
        if(assetSystem.modelPaths[i] == path)
        {
            foundIdx = i;
            break;
        }
    }
    
    if(foundIdx != -1) return assetSystem.models[foundIdx];
    
    Model* res = LoadModel(path);
    Append(&assetSystem.modelPaths, path);
    Append(&assetSystem.models, res);
    return res;
}

R_Texture GetTextureByPath(const char* path)
{
    int foundIdx = -1;
    for(int i = 0; i < assetSystem.texturePaths.len; ++i)
    {
        if(assetSystem.texturePaths[i] == path)
        {
            foundIdx = i;
            break;
        }
    }
    
    if(foundIdx != -1) return assetSystem.textures[foundIdx];
    
    R_Texture res = LoadTexture(path);
    Append(&assetSystem.texturePaths, path);
    Append(&assetSystem.textures, res);
    return res;
}

R_Shader GetShaderByPath(const char* path, ShaderKind kind)
{
    int foundIdx = -1;
    for(int i = 0; i < assetSystem.shaderPaths.len; ++i)
    {
        if(assetSystem.shaderPaths[i] == path)
        {
            foundIdx = i;
            break;
        }
    }
    
    if(foundIdx != -1) return assetSystem.shaders[foundIdx];
    
    R_Shader res = LoadShader(path, kind);
    Append(&assetSystem.shaderPaths, path);
    Append(&assetSystem.shaders, res);
    return res;
}

R_Cubemap GetCubemapByPath(const char* topPath, const char* bottomPath, const char* leftPath, const char* rightPath, const char* frontPath, const char* backPath)
{
    int foundIdx = -1;
    for(int i = 0; i < assetSystem.cubemapPaths.len; ++i)
    {
        if(assetSystem.cubemapPaths[i].topPath == topPath &&
           assetSystem.cubemapPaths[i].bottomPath == bottomPath &&
           assetSystem.cubemapPaths[i].leftPath == leftPath &&
           assetSystem.cubemapPaths[i].rightPath == rightPath &&
           assetSystem.cubemapPaths[i].frontPath == frontPath &&
           assetSystem.cubemapPaths[i].backPath == backPath)
        {
            foundIdx = i;
            break;
        }
    }
    
    if(foundIdx != -1) return assetSystem.cubemaps[foundIdx];
    
    String topTex, bottomTex, leftTex, rightTex, frontTex, backTex;
    int width, height, numChannels;
    
    // @tmp
    {
        topTex = {0};
        int curWidth, curHeight, curNumChannels;
        topTex.ptr = (char*)stbi_load(topPath, &curWidth, &curHeight, &curNumChannels, 0);
        topTex.len = curWidth * curHeight * curNumChannels;
        if(!topTex.ptr)
        {
            // Fallback
            // TODO: Fallback texture
            TODO;
        }
        
        width = curWidth;
        height = curHeight;
        numChannels = curNumChannels;
    }
    
    {
        bottomTex = {0};
        int curWidth, curHeight, curNumChannels;
        bottomTex.ptr = (char*)stbi_load(bottomPath, &curWidth, &curHeight, &curNumChannels, 0);
        bottomTex.len = curWidth * curHeight * curNumChannels;
        if(!bottomTex.ptr)
        {
            // Fallback
            // TODO: Fallback texture
            TODO;
        }
        assert(width == curWidth && height == curHeight && numChannels == curNumChannels);
    }
    
    {
        rightTex = {0};
        int curWidth, curHeight, curNumChannels;
        rightTex.ptr = (char*)stbi_load(rightPath, &curWidth, &curHeight, &curNumChannels, 0);
        rightTex.len = curWidth * curHeight * curNumChannels;
        if(!rightTex.ptr)
        {
            // Fallback
            // TODO: Fallback texture
            TODO;
        }
        assert(width == curWidth && height == curHeight && numChannels == curNumChannels);
    }
    
    {
        leftTex = {0};
        int curWidth, curHeight, curNumChannels;
        leftTex.ptr = (char*)stbi_load(leftPath, &curWidth, &curHeight, &curNumChannels, 0);
        leftTex.len = curWidth * curHeight * curNumChannels;
        if(!leftTex.ptr)
        {
            // Fallback
            // TODO: Fallback texture
            TODO;
        }
        assert(width == curWidth && height == curHeight && numChannels == curNumChannels);
    }
    
    {
        frontTex = {0};
        int curWidth, curHeight, curNumChannels;
        frontTex.ptr = (char*)stbi_load(frontPath, &curWidth, &curHeight, &curNumChannels, 0);
        frontTex.len = curWidth * curHeight * curNumChannels;
        if(!frontTex.ptr)
        {
            // Fallback
            // TODO: Fallback texture
            TODO;
        }
        assert(width == curWidth && height == curHeight && numChannels == curNumChannels);
    }
    
    {
        backTex = {0};
        int curWidth, curHeight, curNumChannels;
        backTex.ptr = (char*)stbi_load(backPath, &curWidth, &curHeight, &curNumChannels, 0);
        backTex.len = curWidth * curHeight * curNumChannels;
        if(!backTex.ptr)
        {
            // Fallback
            // TODO: Fallback texture
            TODO;
        }
        assert(width == curWidth && height == curHeight && numChannels == curNumChannels);
    }
    
    R_Cubemap res = R_UploadCubemap(topTex, bottomTex, leftTex, rightTex, frontTex, backTex, width, height, numChannels);
    Append(&assetSystem.cubemapPaths, {topPath, bottomPath, leftPath, rightPath, frontPath, backPath});
    Append(&assetSystem.cubemaps, res);
    
    stbi_image_free((void*)topTex.ptr);
    stbi_image_free((void*)bottomTex.ptr);
    stbi_image_free((void*)leftTex.ptr);
    stbi_image_free((void*)rightTex.ptr);
    stbi_image_free((void*)frontTex.ptr);
    stbi_image_free((void*)backTex.ptr);
    
    return res;
}

R_Pipeline GetPipelineByPath(const char* vertShaderPath, const char* pixelShaderPath)
{
    int foundIdx = -1;
    for(int i = 0; i < assetSystem.pipelinePaths.len; ++i)
    {
        if(assetSystem.pipelinePaths[i].vertPath == vertShaderPath &&
           assetSystem.pipelinePaths[i].pixelPath == pixelShaderPath)
        {
            foundIdx = i;
            break;
        }
    }
    
    if(foundIdx != -1) return assetSystem.pipelines[foundIdx];
    
    R_Shader shaders[] =
    {
        GetShaderByPath(vertShaderPath,  ShaderKind_Vertex),
        GetShaderByPath(pixelShaderPath, ShaderKind_Pixel)
    };
    
    R_Pipeline res = R_CreatePipeline(ArrToSlice(shaders));
    Append(&assetSystem.pipelinePaths, {vertShaderPath, pixelShaderPath});
    Append(&assetSystem.pipelines, res);
    return res;
}

Material* GetMaterialByPath(const char* path)
{
    int foundIdx = -1;
    for(int i = 0; i < assetSystem.materialPaths.len; ++i)
    {
        if(assetSystem.materialPaths[i] == path)
        {
            foundIdx = i;
            break;
        }
    }
    
    if(foundIdx != -1) return assetSystem.materials[foundIdx];
    
    Material* res = LoadMaterial(path);
    Append(&assetSystem.materialPaths, path);
    Append(&assetSystem.materials, res);
    return res;
}

Model* GetModel(AssetKey key)
{
    return GetModelByPath(key.path);
}

R_Texture GetTexture(AssetKey key)
{
    return GetTextureByPath(key.path);
}

Material* GetMaterial(AssetKey key)
{
    return GetMaterialByPath(key.path);
}

// Dealing with text files
Token GetNextToken(Tokenizer* t)
{
    EatAllWhitespace(t);
    
    Token token = {0};
    token.kind = Tok_EndOfStream;
    token.lineNum = t->lineNum;
    token.startPos = (int)(t->at - t->lineStart);
    token.text = {.ptr=t->at, .len=1};
    
    switch(*t->at)
    {
        case '\0': token.kind = Tok_EndOfStream;  ++t->at; break;
        case '(':  token.kind = Tok_OpenParen;    ++t->at; break;
        case ')':  token.kind = Tok_CloseParen;   ++t->at; break;
        case '[':  token.kind = Tok_OpenBracket;  ++t->at; break;
        case ']':  token.kind = Tok_CloseBracket; ++t->at; break;
        case '{':  token.kind = Tok_OpenBrace;    ++t->at; break;
        case '}':  token.kind = Tok_CloseBrace;   ++t->at; break;
        case ':':  token.kind = Tok_Colon;        ++t->at; break;
        case ';':  token.kind = Tok_Semicolon;    ++t->at; break;
        case '*':  token.kind = Tok_Asterisk;     ++t->at; break;
        
        case '"':
        {
            token.kind = Tok_String;
            
            ++t->at;
            token.text.ptr = t->at;
            while(t->at[0] != '\0' && t->at[0] != '"')
            {
                if(t->at[0] == '\\' && t->at[1] != '\0')
                    ++t->at;
                
                ++t->at;
            }
            
            token.text.len = t->at - token.text.ptr;
            
            if(t->at[0] == '"')
                ++t->at;
            break;
        }
        
        default:
        {
            if(IsAlpha(t->at[0]))
            {
                token.kind = Tok_Ident;
                
                while(IsAlpha(t->at[0]) || IsNumber(t->at[0]) || t->at[0] == '_')
                    ++t->at;
                
                token.text.len = t->at - token.text.ptr;
            }
#if 0
            else if(IsNumber(t->at[0]))
            {
                
            }
#endif
            else
            {
                token.kind = Tok_Unknown;
                ++t->at;
            }
            
            break;
        }
    }
    
    return token;
}

void EatAllWhitespace(Tokenizer* t)
{
    while(true)
    {
        if(*t->at == '\n')
        {
            ++t->lineNum;
            t->lineStart = t->at;
            ++t->at;
        }
        else if(*t->at == '/' && *t->at == '/')  // Single line comment
        {
            while(*t->at != '\n' && *t->at != 0) ++t->at;
            
            if(*t->at == '\n')
            {
                ++t->lineNum;
                t->lineStart = t->at;
                ++t->at;
            }
        }
        else if(t->at[0] == '/' && t->at[1] == '*')  // Multiline comment
        {
            while((t->at[0] != '*' || t->at[1] != '/') && *t->at != 0)
            {
                if(*t->at == '\n')
                {
                    ++t->lineNum;
                    t->lineStart = t->at;
                }
                
                ++t->at;
            }
        }
        else if(IsWhitespace(*t->at))
            ++t->at;
        else
            break;
    }
}

bool IsWhitespace(char c)
{
    return c == '\t' || c == ' ' || c == '\n' || c == '\r';
}

bool IsAlpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool IsNumber(char c)
{
    return c >= '0' && c <= '9';
}
