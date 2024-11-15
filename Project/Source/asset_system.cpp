
#include "base.h"
#include "asset_system.h"
#include "parser.h"

static AssetSystem assetSystem = {};

void AssetSystemInit()
{
    auto& sys = assetSystem;
    //sys.defaultAssets[Asset_Mesh];
    TODO;
}


Mesh*        GetAsset(MeshHandle handle)        { return &assetSystem.assets[handle.slot].mesh;     }
R_Shader*    GetAsset(VertShaderHandle handle)  { return &assetSystem.assets[handle.slot].shader;   }
R_Shader*    GetAsset(PixelShaderHandle handle) { return &assetSystem.assets[handle.slot].shader;   }
Material*    GetAsset(MaterialHandle handle)    { return &assetSystem.assets[handle.slot].material; }
R_Texture2D* GetAsset(Texture2DHandle handle)   { return &assetSystem.assets[handle.slot].texture2D; }
//R_Cubemap*   GetAsset(CubemapHandle handle)     { return &assetSystem.assets[handle.slot].cubemap;   }

static u32 AcquireAsset(AssetKind kind, String path, bool* outNew)
{
    auto& sys = assetSystem;
    auto lookup = Lookup(&sys.pathMapping, path);
    if(lookup.found)
    {
        *outNew = false;
        
        auto value = lookup.res;
        assert(value->kind == kind);
        
        auto& asset = sys.assets[value->slot];
        return value->slot;
    }
    else
    {
        *outNew = true;
        
        u32 slot = 0;
        if(sys.freeSlots.len > 0)
        {
            slot = sys.freeSlots[sys.freeSlots.len - 1];
            Pop(&sys.freeSlots);
        }
        else
        {
            Append(&sys.assets, {});
            slot = sys.assets.len - 1;
        }
        
        
        Append(&sys.pathMapping, path, {kind, slot});
        
        auto& asset = sys.assets[slot];
        asset.kind = kind;
        asset.slot = slot;
        
        return slot;
    }
}

static void ReleaseAsset(AssetKind kind, AssetHandle handle)
{
    auto& sys = assetSystem;
    assert(handle.slot < (u32)sys.assets.len);
    
    auto& asset = sys.assets[handle.slot];
    assert(kind == asset.kind);
    
    
    //assert(asset.refCount > 0 && "Trying to release an asset which has already been released");
    
    //--asset.refCount;
    //if(asset.refCount == 0)
    if(false)
    {
        // TODO: Free the resource, and also free its entry in the pathMapping table
    }
}

MeshHandle AcquireMesh(String path)
{
    bool newAsset = false;
    u32 slot = AcquireAsset(Asset_Mesh, path, &newAsset);
    bool ok = true;
    if(newAsset)
    {
        auto asset = LoadMesh(path, &ok);
        assetSystem.assets[slot].mesh = asset;
    }
    return {slot};
}

VertShaderHandle AcquireVertShader(String path)
{
    bool newAsset = false;
    u32 slot = AcquireAsset(Asset_VertShader, path, &newAsset);
    bool ok = true;
    if(newAsset)
    {
        auto asset = LoadShader(path, ShaderType_Vertex, &ok);
        assetSystem.assets[slot].shader = asset;
    }
    return {slot};
}

PixelShaderHandle AcquirePixelShader(String path)
{
    bool newAsset = false;
    u32 slot = AcquireAsset(Asset_PixelShader, path, &newAsset);
    bool ok = true;
    if(newAsset)
    {
        auto asset = LoadShader(path, ShaderType_Pixel, &ok);
        assetSystem.assets[slot].shader = asset;
    }
    return {slot};
}

MaterialHandle AcquireMaterial(String path)
{
    bool newAsset = false;
    u32 slot = AcquireAsset(Asset_Material, path, &newAsset);
    bool ok = true;
    if(newAsset)
    {
        auto asset = LoadMaterial(path, &ok);
        assetSystem.assets[slot].material = asset;
    }
    return {slot};
}

Texture2DHandle AcquireTexture2D(String path)
{
    bool newAsset = false;
    u32 slot = AcquireAsset(Asset_Texture2D, path, &newAsset);
    bool ok = false;
    if(newAsset)
    {
        auto asset = LoadTexture2D(path, &ok);
        assetSystem.assets[slot].texture2D = asset;
    }
    return {slot};
}

#if 0
CubemapHandle AcquireCubemap(String path)
{
    u32 slot = AcquireAsset(Asset_Cubemap, path);
    void* asset = &assetSystem.assets[slot].content;
    LoadCubemap((R_Cubemap*)asset, path);
    return {slot};
}
#endif

MeshHandle AcquireMesh(const char* path)                { return AcquireMesh(ToLenStr(path));        }
VertShaderHandle AcquireVertShader(const char* path)    { return AcquireVertShader(ToLenStr(path));  }
PixelShaderHandle AcquirePixelShader(const char* path)  { return AcquirePixelShader(ToLenStr(path)); }
MaterialHandle AcquireMaterial(const char* path)        { return AcquireMaterial(ToLenStr(path));    }
Texture2DHandle AcquireTexture2D(const char* path)      { return AcquireTexture2D(ToLenStr(path));   }
//CubemapHandle AcquireCubemap(const char* path)        { return AcquireCubemap(ToLenStr(path));     }

//void ReleaseModel(ModelHandle handle)             { ReleaseAsset(Asset_Model, handle);       }
void ReleaseVertShader(VertShaderHandle handle)     { ReleaseAsset(Asset_VertShader, handle);  }
void ReleasePixelShader(PixelShaderHandle handle)   { ReleaseAsset(Asset_PixelShader, handle); }
//void ReleaseMaterial(MaterialHandle handle)       { ReleaseAsset(Asset_Material, handle);    }
//void ReleaseTexture2D(Texture2DHandle handle)     { ReleaseAsset(Asset_Texture2D, handle);   }
//void ReleaseCubemap(CubemapHandle handle)         { ReleaseAsset(Asset_Cubemap, handle);     }

R_Shader LoadShader(String path, ShaderType type, bool* ok)
{
    ScratchArena scratch;
    \
    bool success = true;
    String contents = LoadEntireFile(path, scratch, &success);
    if(!success)
    {
        Log("Failed to load file '%.*s'\n", StrPrintf(path));
        *ok = false;
        return {};
    }
    
    char** cursor;
    char* c = (char*)contents.ptr;
    cursor = &c;
    
    String magicBytes = Next(cursor, sizeof("shader")-1);  // Excluding null terminator
    if(magicBytes != "shader")
    {
        Log("Attempted to load file '%.*s' as a shader, which it is not.", StrPrintf(path));
        *ok = false;
        return {};
    }
    
    u32 version = Next<u32>(cursor);
    if(version < 0 || version > 1)
    {
        Log("Attempted to load file '%.*s' as a shader, but its version is unsupported.", StrPrintf(path));
        *ok = false;
        return {};
    }
    
    char* headerPtr = *cursor;
    ShaderBinaryHeader_v0 header_v0 = {};
    header_v0 = Next<ShaderBinaryHeader_v0>(cursor);
    
    auto& header = header_v0;
    if(header.shaderType != type)
    {
        const char* desiredKindStr = GetShaderTypeString(type);
        const char* actualKindStr  = GetShaderTypeString((ShaderType)header.shaderType);
        Log("Attempted to load shader '%.*s' as a %s, but it's a %s", StrPrintf(path), desiredKindStr, actualKindStr);
        *ok = false;
        return {};
    }
    
    R_ShaderInput input = {};
    input.dxil          = {.ptr=headerPtr+header.dxil, .len=header.dxilSize};
    input.vulkanSpirv   = {.ptr=headerPtr+header.vulkanSpirv, .len=header.vulkanSpirvSize};
    input.glsl          = {.ptr=headerPtr+header.glsl, .len=header.glslSize};
    input.d3d11Bytecode = {.ptr=headerPtr+header.d3d11Bytecode, .len=header.d3d11BytecodeSize};
    auto shader = R_ShaderAlloc(input, (ShaderType)header.shaderType);
    return shader;
}

Mesh LoadMesh(String path, bool* ok)
{
    ScratchArena scratch;
    
    bool success = true;
    String contents = LoadEntireFile(path, scratch, &success);
    if(!success)
    {
        Log("Failed to load file '%.*s'", StrPrintf(path));
        *ok = false;
        return {};
    }
    
    char** cursor;
    char* c = (char*)contents.ptr;
    cursor = &c;
    
    String magicBytes = Next(cursor, sizeof("mesh")-1);  // Excluding null terminator
    if(magicBytes != "mesh")
    {
        Log("Attempted to load file '%.*s' as a mesh, which it is not.", StrPrintf(path));
        *ok = false;
        return {};
    }
    
    u32 version = Next<u32>(cursor);
    if(version != 0)
    {
        Log("Attempted to load file '%.*s' as a mesh, but its version is unsupported.", StrPrintf(path));
        *ok = false;
        return {};
    }
    
    char* headerPtr = *cursor;
    auto header = Next<MeshHeader_v0>(cursor);
    
    if(header.isSkinned)
    {
        Log("Skinned meshes are not yet supported.");
        *ok = false;
        return {};
    }
    
    Slice<Vertex> verts   = {(Vertex*)(headerPtr + header.vertsOffset),   header.numVerts};
    Slice<u32>    indices = {(u32*)   (headerPtr + header.indicesOffset), header.numIndices};
    auto mesh = StaticMeshAlloc({verts, indices});
    return mesh;
}

Material LoadMaterial(String path, bool* ok)
{
    ScratchArena scratch;
    
    TextFileHandler handler = LoadTextFile(path, scratch);
    if(!handler.ok)
    {
        Log("Could not load material '%.*s'", StrPrintf(path));
        *ok = false;
        return {};
    }
    
    Material mat = {};
    while(true)
    {
        auto line = ConsumeNextLine(&handler);
        if(!line.ok) break;
        
        if(line.text == ":/material")
        {
            while(true)
            {
                auto matLine = GetNextLine(&handler);
                if(!matLine.ok) break;
                
                auto strings = BreakByChar(matLine, ':');
                if(strings.a == "pixel_shader")
                {
                    mat.shader = AcquirePixelShader(strings.b);
                    ConsumeNextLine(&handler);
                }
                else
                    break;
            }
        }
        else if(line.text == ":/textures")
        {
            while(true)
            {
                auto texLine = GetNextLine(&handler);
                if(!texLine.ok) break;
                if(StringBeginsWith(texLine.text, ":/")) break;
                
                ConsumeNextLine(&handler);
                Texture2DHandle handle = AcquireTexture2D(texLine.text);
                Append(&mat.textures, handle);
            }
        }
        else if(line.text == ":/constants")
        {
            while(true)
            {
                auto texLine = GetNextLine(&handler);
                if(!texLine.ok) break;
                if(StringBeginsWith(texLine.text, ":/")) break;
                
                ConsumeNextLine(&handler);
                
                TODO;
                
                //auto uniform = ParseUniform(texLine.text);
            }
        }
        else
        {
            Log("Error in material '%.*s' (line %d): expecting ':/material', ':/textures', or ':/uniforms'", StrPrintf(path), line.num);
            *ok = false;
            break;
        }
    }
    
#ifdef Development
    // TODO: Maybe check material correctness
    
#endif
    
    return mat;
}

R_Texture2D LoadTexture2D(String path, bool* ok)
{
    ScratchArena scratch;
    
    bool success = true;
    String contents = LoadEntireFile(path, scratch, &success);
    if(!success)
    {
        Log("Failed to load texture '%.*s'", StrPrintf(path));
        *ok = false;
        return {};
    }
    
    String stbImage = {0};
    int width, height, numChannels;
    stbImage.ptr = (char*)stbi_load_from_memory((const stbi_uc*)contents.ptr, (int)contents.len, &width, &height, &numChannels, 4);
    stbImage.len = width * height * numChannels;
    defer { stbi_image_free((void*)stbImage.ptr); };
    
    if(!stbImage.ptr)
    {
        Log("Failed to load texture '%.*s'", StrPrintf(path));
        *ok = false;
        return {};
    }
    
    auto tex = R_Texture2DAlloc(TextureFormat_RGBA_SRGB,
                                width, height, (void*)stbImage.ptr);
    return tex;
}

#if 0

void LoadCubemap(R_Texture* cubemap, String path)
{
    ScratchArena scratch;
    
    const char* paths[6] = {0};
    
    // NOTE: These need to match the call at the bottom
    const char* addendums[6] =
    {
        "_top", "_bottom", "_left", "_right", "_front", "_back"
    };
    
    String ext = GetPathExtension(path);
    
    for(int i = 0; i < 6; ++i)
    {
        StringBuilder builder = {0};
        UseArena(&builder, scratch);
        String pathNoExt = GetPathNoExtension(path);
        Append(&builder, pathNoExt);
        Append(&builder, addendums[i]);
        Append(&builder, '.');
        Append(&builder, ext);
        NullTerminate(&builder);
        
        paths[i] = ToString(&builder).ptr;
    }
    
    String textures[6] = {0};    // If length is zero, it means that the texture was not found
    bool isPlaceholder[6] = {0}; // If given texture was loaded with stb or if it comes from an arena
    int width = 0, height = 0, numChannels = 0;
    
    for(int i = 0; i < 6; ++i)
    {
        // TODO: Support channels with fewer than 4 channels
        int prevWidth = width;
        int prevHeight = height;
        int prevNumChannels = numChannels;
        textures[i].ptr = (char*)stbi_load(paths[i], &width, &height, &numChannels, 4);
        if(!textures[i].ptr)
        {
            Log("Could not load texture '%s' for cubemap", paths[i]);
            isPlaceholder[i] = true;
        }
        else
        {
            bool isDifferentFormat = prevWidth != width && prevHeight != height && prevNumChannels != numChannels;
            if(prevWidth != 0 && isDifferentFormat)
            {
                Log("Textures in cubemap '%s' have different formats", paths[i]);
                isPlaceholder[i] = true;
            }
        }
    }
    
    // At this point, if no texture was found, just use default values
    if(width == 0)
    {
        width = height = 1;
        numChannels = 3;
    }
    
    // Prepare all placeholders
    for(int i = 0; i < 6; ++i)
    {
        if(isPlaceholder[i])
        {
            s64 size = width * height * numChannels;
            u8* ptr = (u8*)ArenaAlloc(scratch, size, 1);
            for(int i = 0; i < size; i += 3)
            {
                ptr[i+0] = 255;
                ptr[i+1] = 0;
                ptr[i+2] = 255;
            }
            
            textures[i].ptr = (const char*)ptr;
            textures[i].len = width * height * numChannels;
        }
    }
    
    // NOTE: This needs to match the array at the top
    *cubemap = R_UploadCubemap(textures[0], textures[1], textures[2],
                               textures[3], textures[4], textures[5],
                               width, height, numChannels);
}

#endif
