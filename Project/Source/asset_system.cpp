
#include "asset_system.h"
#include "parser.h"

static Arena sceneArena = ArenaVirtualMemInit(GB(4), MB(2));

static AssetSystem assetSystem;

Model* LoadModelAssetByName(const char* name)
{
    // This will do the name->path mapping
    TODO;
    return nullptr;
}

void MaybeReloadModelAsset(Model* model)
{
    
}

void LoadScene(const char* path)
{
    
}

Model* LoadModel(const char* path)
{
    ScratchArena scratch;
    
    auto res = ArenaZAllocTyped(Model, &sceneArena);
    
    bool success = true;
    String contents = LoadEntireFile(path, &success);
    if(!success)
    {
        Log("Failed to log file '%s'", path);
        
        // res->meshes.len is already 0, so it's
        // already sufficient for a fallback model
        return res;
    }
    
    defer { free((void*)contents.ptr); };
    
    char** cursor;
    char* c = (char*)contents.ptr;
    cursor = &c;
    
    String magicBytes = Next(cursor, sizeof("model")-1);  // Excluding null terminator
    // TODO: Error reporting
    if(magicBytes != "model")
    {
        Log("Attempted to load file '%s' as a model, which it is not.", path);
        
        // res->meshes.len is already 0, so it's
        // already sufficient for a fallback model
        return res;
    }
    
    s32 version        = Next<s32>(cursor);
    s32 numMeshes      = Next<s32>(cursor);
    s32 numMaterials   = Next<s32>(cursor);
    res->meshes.ptr    = ArenaZAllocArray(Mesh, numMeshes, &sceneArena);
    res->meshes.len    = numMeshes;
    
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

R_Shader LoadShader(const char* path)
{
    ScratchArena scratch;
    
    bool success = true;
    String contents = LoadEntireFile(path, scratch, &success);
    if(!success)
    {
        Log("Failed to load file '%s'\n", path);
        
        // Fallback
        // TODO: Fallback shader
        TODO;
        return {0};
    }
    
    char** cursor;
    char* c = (char*)contents.ptr;
    cursor = &c;
    
    String magicBytes = Next(cursor, sizeof("shader")-1);  // Excluding null terminator
    if(magicBytes != "shader")
    {
        Log("Attempted to load file '%s' as a shader, which it is not.", path);
        
        TODO;
        return {0};
    }
    
    u32 version = Next<u32>(cursor);
    if(version != 0)
    {
        Log("Attempted to load file '%s' as a shader, but its version is unsupported.", path);
        TODO;
        return {0};
    }
    
    char* headerPtr = *cursor;
    ShaderBinaryHeader_v0 header = Next<ShaderBinaryHeader_v0>(cursor);
    
    String dxil        = {.ptr=headerPtr+header.dxil, .len=header.dxilSize};
    String vulkanSpirv = {.ptr=headerPtr+header.vulkanSpirv, .len=header.vulkanSpirvSize};
    String glsl        = {.ptr=headerPtr+header.glsl, .len=header.glslSize};
    return R_CompileShader((ShaderKind)header.shaderKind, dxil, vulkanSpirv, glsl);
}

void LoadAssetBinding(const char* path)
{
    TODO;
}

void LoadMaterial(const char* path)
{
    TODO;
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

R_Shader GetShaderByPath(const char* path)
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
    
    R_Shader res = LoadShader(path);
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
        GetShaderByPath(vertShaderPath),
        GetShaderByPath(pixelShaderPath)
    };
    
    R_Pipeline res = R_CreatePipeline(ArrToSlice(shaders));
    Append(&assetSystem.pipelinePaths, {vertShaderPath, pixelShaderPath});
    Append(&assetSystem.pipelines, res);
    return res;
}

// Dealing with text files
inline bool IsStartIdent(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

inline bool IsNumeric(char c)
{
    return c >= '0' && c <= '9';
}

inline bool IsMiddleIdent(char c)
{
    return IsStartIdent(c) || IsNumeric(c);
}

inline bool IsWhitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n';
}

int EatAllWhitespace(char** at)
{
    int newlines = 0;
    int commentNestLevel = 0;
    bool inSingleLineComment = false;
    while(true)
    {
        if(**at == '\n')
        {
            inSingleLineComment = false;
            ++newlines;
            ++*at;
        }
        else if((*at)[0] == '/' && (*at)[1] == '/')
        {
            if(commentNestLevel == 0) inSingleLineComment = true;
            *at += 2;
        }
        else if((*at)[0] == '/' && (*at)[1] == '*')  // Multiline comment start
        {
            *at += 2;
            ++commentNestLevel;
        }
        else if((*at)[0] == '*' && (*at)[1] == '/')  // Multiline comment end
        {
            *at += 2;
            --commentNestLevel;
        }
        else if(IsWhitespace(**at) || commentNestLevel > 0 || inSingleLineComment)
        {
            ++*at;
        }
        else
            break;
    }
    
    return newlines;
}

Token NextToken(Tokenizer* tokenizer)
{
    Tokenizer* t = tokenizer;
    
    if(t->error)
    {
        Token errorToken = {0};
        errorToken.lineNum = 0;
        errorToken.kind = TokKind_Error;
        return errorToken;
    }
    
    int newlines = EatAllWhitespace(&t->at);
    t->numLines += newlines;
    
    if(!t->at || *t->at == '\0')
    {
        Token endToken = {0};
        endToken.lineNum = t->numLines;
        endToken.kind = TokKind_EOF;
        return endToken;
    }
    
    Token token = {0};
    token.lineNum = t->numLines;
    
    if(IsStartIdent(*t->at))
    {
        char* startIdent = t->at;
        ++t->at;
        while(IsMiddleIdent(*t->at)) ++t->at;
        
        String text = {.ptr=startIdent, .len=t->at-startIdent};
        token.text = text;
        
        // Check if it's a keyword
        bool isKeyword = true;
        if(text == "true")
        {
            token.kind = TokKind_True;
        }
        else if(text == "false")
        {
            token.kind = TokKind_False;
        }
        else if(text == "material_name")
        {
            token.kind = TokKind_MaterialName;
        }
        else if(text == "vertex_shader")
        {
            token.kind = TokKind_VertexShader;
        }
        else if(text == "pixel_shader")
        {
            token.kind = TokKind_PixelShader;
        }
        else if(text == "values")
        {
            token.kind = TokKind_Values;
        }
        else
            isKeyword = false;
        
        if(!isKeyword)
        {
            token.kind = TokKind_Ident;
        }
    }
    else if(IsNumeric(*t->at))
    {
        TODO;
        // Find out if float value or int value
    }
    else  // Operators and other miscellaneous things
    {
        char* start = t->at;
        int textLen = 0;
        
        if(*t->at == ':')
        {
            token.kind = TokKind_Colon;
            textLen = 1;
        }
        else if(*t->at == ',')
        {
            token.kind = TokKind_Comma;
            textLen = 1;
        }
        else if(*t->at == '.')
        {
            token.kind = TokKind_Dot;
            textLen = 1;
        }
        else
        {
            token.kind = TokKind_Error;
            textLen = 1;
        }
        
        token.text = {.ptr=t->at, .len=textLen};
        t->at += token.text.len;
    }
    
    return token;
}