
#include "asset_system.h"
#include "parser.h"

// NOTE: I think the overall design should be this:
// when a user gets an asset in any way, that should be valid
// for the entire duration of the current frame. Only in a frame
// boundary, these can get swapped. If the user needs to hold on to
// an asset for more than one frame, it needs to use an AssetHandle.
// When using an AssetHandle, you also get hot reloading for free.
// I think this module should provide functions to load/use simple models and simple assets,
// as well. It should not be the renderer's responsibility (except for the things that are truly platform dependent, such as default shaders, but even then we could probably just use
// an asset for that.

static Arena sceneArena = ArenaVirtualMemInit(GB(4), MB(2));
static AssetSystem assetManager = {};

Material DefaultMaterial()
{
    Material mat = {};
    mat.pixelShader = {};
    mat.uniforms = {};
    mat.textures = {};
    return mat;
}

// Implicit casts
MeshHandle::operator R_Mesh()       { return assetManager.meshes[idx];    }
TextureHandle::operator R_Texture() { return assetManager.textures[idx];  }
ShaderHandle::operator R_Shader()   { return assetManager.shaders[idx];   }
MaterialHandle::operator Material() { return assetManager.materials[idx]; }

void InitAssetSystem()
{
    ReserveSlotForDefaultAssets();
}

void ReserveSlotForDefaultAssets()
{
    auto& man = assetManager;
    
    assert(man.meshes.len == 0);
    Append(&man.meshes, R_CreateDefaultMesh());
    
    assert(man.textures.len == 0);
    //Append(&man.textures, R_MakeDefaultTexture());
    
    assert(man.shaders.len == 0);
    Append(&man.shaders, R_CreateDefaultShader(ShaderKind_Vertex));
    Append(&man.shaders, R_CreateDefaultShader(ShaderKind_Pixel));
}

void UseMaterial(Material mat)
{
    R_SetPixelShader(mat.pixelShader);
    R_SetMaterialConstants(mat.pixelShader, ToSlice(&mat.uniforms));
    
    for(int i = 0; i < mat.textures.len; ++i)
        R_SetTexture(mat.textures[i], ShaderKind_Pixel, (TextureSlot)(MaterialTex0 + i));
}

void HotReloadAssets(Arena* frameArena)
{
#ifdef Development
    auto& man = assetManager;
    
    // Maybe some of this duplication can be reduced...
    Slice<OS_FileSystemChange> fileChanges = OS_ConsumeFileWatcherChanges(frameArena);
    for(int i = 0; i < fileChanges.len; ++i)
    {
        String ext = GetPathExtension(fileChanges[i].file);
        if(ext == "mesh")
        {
            auto lookup = Lookup(&man.pathToMesh, fileChanges[i].file);
            if(lookup.found)
            {
                // TODO: Destroy mesh
                
                auto handle = lookup.res;
                LoadMesh(&man.meshes[handle->idx], ToLenStr(fileChanges[i].file));
            }
        }
        else if(ext == "png" || ext == "jpg" || ext == "jpeg")
        {
            auto lookup = Lookup(&man.pathToTexture, fileChanges[i].file);
            if(lookup.found)
            {
                // TODO: Destroy texture
                
                auto handle = lookup.res;
                LoadTexture(&man.textures[handle->idx], ToLenStr(fileChanges[i].file));
            }
        }
        else if(ext == "shader")
        {
            TODO;
        }
        else if(ext == "mat")
        {
            auto lookup = Lookup(&man.pathToMaterial, fileChanges[i].file);
            if(lookup.found)
            {
                // TODO: Destroy material
                
                auto handle = lookup.res;
                LoadMaterial(&man.materials[handle->idx], ToLenStr(fileChanges[i].file));
            }
        }
    }
#endif
}

void LoadMesh(R_Mesh* mesh, String path)
{
    ScratchArena scratch;
    
    bool success = true;
    String contents = LoadEntireFile(path, scratch, &success);
    if(!success)
    {
        Log("Failed to load file '%.*s'", StrPrintf(path));
        *mesh = R_CreateDefaultMesh();
        return;
    }
    
    char** cursor;
    char* c = (char*)contents.ptr;
    cursor = &c;
    
    String magicBytes = Next(cursor, sizeof("mesh")-1);  // Excluding null terminator
    if(magicBytes != "mesh")
    {
        Log("Attempted to load file '%.*s' as a mesh, which it is not.", StrPrintf(path));
        *mesh = R_CreateDefaultMesh();
        return;
    }
    
    u32 version = Next<u32>(cursor);
    if(version != 0)
    {
        Log("Attempted to load file '%.*s' as a mesh, but its version is unsupported.", StrPrintf(path));
        *mesh = R_CreateDefaultMesh();
        return;
    }
    
    char* headerPtr = *cursor;
    auto header = Next<MeshHeader_v0>(cursor);
    
    if(header.isSkinned)
    {
        Log("Skinned meshes are not yet supported.");
        *mesh = R_CreateDefaultMesh();
        return;
    }
    
    Slice<Vertex> verts   = {(Vertex*)(headerPtr + header.vertsOffset),   header.numVerts};
    Slice<s32>    indices = {(s32*)   (headerPtr + header.indicesOffset), header.numIndices};
    *mesh = R_UploadMesh(verts, indices);
}

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

void LoadMesh(const char* path)
{
    String str = ToLenStr(path);
    Append(&assetManager.meshes, {});
    String newStr = ArenaPushString(&sceneArena, path);
    Append(&assetManager.pathToMesh, newStr, {(u32)assetManager.meshes.len - 1});
    LoadMesh(&assetManager.meshes[assetManager.meshes.len - 1], str);
}

R_Texture CreateDefaultTexture()
{
    u8 texData[] = {255, 0, 255};
    String s = {.ptr=(const char*)texData, .len=sizeof(texData)};
    return R_UploadTexture(s, 1, 1, 3);
}

void LoadTexture(R_Texture* texture, String path)
{
    ScratchArena scratch;
    
    bool success = true;
    String contents = LoadEntireFile(path, scratch, &success);
    if(!success)
    {
        Log("Failed to load texture '%.*s'", StrPrintf(path));
        *texture = CreateDefaultTexture();
        return;
    }
    
    // TODO: Support textures with fewer than 4 channels
    String stbImage = {0};
    int width, height, numChannels;
    stbImage.ptr = (char*)stbi_load_from_memory((const stbi_uc*)contents.ptr, (int)contents.len, &width, &height, &numChannels, 4);
    stbImage.len = width * height * numChannels;
    if(!stbImage.ptr)
    {
        Log("Failed to load texture '%.*s'", StrPrintf(path));
        *texture = CreateDefaultTexture();
        return;
    }
    
    *texture = R_UploadTexture(stbImage, width, height, numChannels);
    stbi_image_free((void*)stbImage.ptr);
    return;
}

MeshHandle GetMeshByPath(const char* path)
{
    auto& man = assetManager;
    
    auto res = Lookup(&man.pathToMesh, ToLenStr(path));
    if(!res.found)
    {
        MeshHandle mesh;
        Append(&man.meshes, {});
        mesh.idx = man.meshes.len - 1;
        LoadMesh(&man.meshes[mesh.idx], ToLenStr(path));
        String newStr = ArenaPushString(&sceneArena, path);
        Append(&man.pathToMesh, newStr, mesh);
        return mesh;
    }
    
    return *res.res;
}

TextureHandle GetTextureByPath(const char* path)
{
    auto& man = assetManager;
    auto res = Lookup(&man.pathToTexture, ToLenStr(path));
    if(!res.found)
    {
        TextureHandle texture;
        Append(&man.textures, {});
        texture.idx = man.textures.len - 1;
        LoadTexture(&man.textures[texture.idx], ToLenStr(path));
        String newStr = ArenaPushString(&sceneArena, path);
        Append(&man.pathToTexture, newStr, texture);
        return texture;
    }
    
    return *res.res;
}

ShaderHandle GetShaderByPath(const char* path, ShaderKind kind)
{
    auto& man = assetManager;
    
    String str = ToLenStr(path);
    auto res = Lookup(&man.pathToShader, str);
    if(!res.found)
    {
        ShaderHandle shader;
        Append(&man.shaders, {});
        shader.idx = man.shaders.len - 1;
        LoadShader(&man.shaders[shader.idx], str, kind);
        String newStr = ArenaPushString(&sceneArena, path);
        Append(&man.pathToShader, newStr, shader);
        return shader;
    }
    
    return *res.res;
}

MaterialHandle GetMaterialByPath(const char* path)
{
    auto& man = assetManager;
    
    String str = ToLenStr(path);
    auto res = Lookup(&man.pathToMaterial, str);
    if(!res.found)
    {
        MaterialHandle material;
        Append(&man.materials, {});
        material.idx = man.materials.len - 1;
        LoadMaterial(&man.materials[material.idx], str);
        String newStr = ArenaPushString(&sceneArena, path);
        Append(&man.pathToMaterial, newStr, material);
        return material;
    }
    
    return *res.res;
}

AssetMetadata GetMetadata(u32 handle, AssetKind kind)
{
#ifdef BoundsChecking
    assert(kind < Asset_Count && kind >= 0);
#endif
    return assetManager.metadata[kind][handle];
}

void LoadShader(R_Shader* shader, String path, ShaderKind kind)
{
    ScratchArena scratch;
    
    bool success = true;
    String contents = LoadEntireFile(path, scratch, &success);
    if(!success)
    {
        Log("Failed to load file '%.*s'\n", StrPrintf(path));
        *shader = R_CreateDefaultShader(kind);
        return;
    }
    
    char** cursor;
    char* c = (char*)contents.ptr;
    cursor = &c;
    
    String magicBytes = Next(cursor, sizeof("shader")-1);  // Excluding null terminator
    if(magicBytes != "shader")
    {
        Log("%.*s", StrPrintf(magicBytes));
        Log("Attempted to load file '%.*s' as a shader, which it is not.", StrPrintf(path));
        *shader = R_CreateDefaultShader(kind);
        return;
    }
    
    u32 version = Next<u32>(cursor);
    if(version < 0 || version > 1)
    {
        Log("Attempted to load file '%.*s' as a shader, but its version is unsupported.", StrPrintf(path));
        *shader = R_CreateDefaultShader(kind);
        return;
    }
    
    char* headerPtr = *cursor;
    ShaderBinaryHeader_v0 header_v0 = {};
    header_v0 = Next<ShaderBinaryHeader_v0>(cursor);
    
#if 0
    // Convert to the final version in an assembly line fashion
    // (first 0 to 1, then 1 to 2, etc)
    switch(version)
    {
        case 0:
        {
            header_v1.v0 = header_v0;
            header_v1.d3d11Bytecode = 0;
            header_v1.d3d11BytecodeSize = 0;
            
        }  // Fallthrough
        case 1:
        {
            // Got to final version.
            // No need to do anything here
            break;
        }
        default: assert(false);
    }
#endif
    
    auto& header = header_v0;
    if(header.shaderKind != kind)
    {
        const char* desiredKindStr = GetShaderKindString(kind);
        const char* actualKindStr  = GetShaderKindString((ShaderKind)header.shaderKind);
        Log("Attempted to load shader '%.*s' as a %s, but it's a %s", StrPrintf(path), desiredKindStr, actualKindStr);
        *shader = R_CreateDefaultShader(kind);
        return;
    }
    
    ShaderInput input = {};
    input.dxil          = {.ptr=headerPtr+header.dxil, .len=header.dxilSize};
    input.vulkanSpirv   = {.ptr=headerPtr+header.vulkanSpirv, .len=header.vulkanSpirvSize};
    input.glsl          = {.ptr=headerPtr+header.glsl, .len=header.glslSize};
    input.d3d11Bytecode = {.ptr=headerPtr+header.d3d11Bytecode, .len=header.d3d11BytecodeSize};
    *shader = R_CreateShader((ShaderKind)header.shaderKind, input);
}

R_UniformValue ParseUniform(String str)
{
    return {};
}

void LoadMaterial(Material* material, String path)
{
    if(path.len <= 0)
    {
        *material = DefaultMaterial();
        return;
    }
    
    ScratchArena scratch;
    
    TextFileHandler handler = LoadTextFile(path, scratch);
    if(!handler.ok)
    {
        Log("Could not load material '%.*s'", StrPrintf(path));
        *material = DefaultMaterial();
        return;
    }
    
    *material = DefaultMaterial();
    
    const char* pixelShaderPath = "";
    
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
                    pixelShaderPath = ArenaPushNullTermString(scratch, strings.b);
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
                String path = ArenaPushString(&sceneArena, texLine.text);
                // TODO
                StringBuilder builder = {};
                UseArena(&builder, scratch);
                Append(&builder, texLine.text);
                NullTerminate(&builder);
                String nullTermStr = ToString(&builder);
                
                TextureHandle handle = GetTextureByPath(nullTermStr.ptr);
                Append(&material->textures, handle);
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
                
                auto uniform = ParseUniform(texLine.text);
            }
        }
        else
        {
            Log("Error in material '%.*s' (line %d): expecting ':/material', ':/textures', or ':/uniforms'", StrPrintf(path), line.num);
            break;
        }
    }
    
    // Set pipeline
    {
        ShaderHandle pixelHandle = {1};  // Default pixel handle @cleanup
        
        if(pixelShaderPath[0] == '\0')
            Log("Error in material '%.*s': pixel shader not specified.", StrPrintf(path));
        else
            pixelHandle = GetShaderByPath(pixelShaderPath, ShaderKind_Pixel);
        
        material->pixelShader = pixelHandle;
    }
    
    // Check material correctness
#ifdef Development
    if(!R_CheckMaterial(material, path))
        *material = DefaultMaterial();  // @leak Don't forget to destroy whatever was created here
#endif
}

void SerializeScene(EntityManager* man, const char* path)
{
    ScratchArena scratch;
    
    // We first want to serialize the currently used assets for our scene
    
    auto bases = man->bases;
    for(int i = 0; i < bases.len; ++i)
    {
        
    }
}

void UnloadScene(EntityManager* entMan)
{
    
}

void LoadScene(EntityManager* man, const char* path)
{
    
}
