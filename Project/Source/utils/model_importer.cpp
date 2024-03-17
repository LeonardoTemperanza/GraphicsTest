
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "os/os_generic.cpp"
#include "base.cpp"

/*

 Animation file format

Version 0:
s32 0
s32 [Framerate]
s32 [Number of bones]
s32 [Number of samples per node]
s32 [Bone names]
                TODO
                */

// Model file format

// Version 0

// All the supported texture types,
// Stored in the format in this order
aiTextureType texTypes[] =
{
    aiTextureType_DIFFUSE,
    aiTextureType_NORMALS
};

// The following specification (pseudocode)
// describes the general data layout of the file
#if 0
struct Model
{
    u8 magicBytes[5] = "model";
    s32 version = 0;
    s32 numMeshes;
    s32 numMaterials;
    
    struct Mesh
    {
        s32 numVerts;
        s32 numIndices;
        s32 materialIdx;
        bool hasTextureCoords;
        Vec3 verts[numVerts];   // AOS
        Vec3 normals[numVerts]; // AOS
        Vec3 textureCoords[numVerts or 0];  // AOS
        s32 indices[numIndices];  // Grouped in 3 to form a triangle
    };
    
    Mesh meshes[numMeshes];
    
    struct MaterialPath
    {
        s32 strLen;
        char str[strLen];
    };
    
    // For now we're doing just vertices.
    //MaterialPath[numMeshes];
};

struct Material
{
    struct TexturePath
    {
        // Path relative to Assets folder
        // if nameLen is 0 it means that it doesn't have this
        // texture type
        s32 nameLen;
        char name[nameLen (which could be 0)];
    };
    
    // Array of textures following 'texTypes'
    TexturePath textures[..];
};

// Version 1...

#endif

struct Material
{
    const aiScene* scene;
    const aiMaterial* mat;
};

std::string RemoveFileExtension(const std::string& fileName);
std::string RemovePathLastPart(const std::string& fileName);

void WriteMaterial(String path, Material mat);
void WriteTexture(String path, Material mat);

// Usage:
// model_importer.exe file_to_import.(obj/fbx/...)
// The path is relative to the Assets folder
int main(int argCount, char** args)
{
    char* exePathCStr = OS_GetExecutablePath();
    std::string exePath = exePathCStr;
    exePath = RemovePathLastPart(exePath);
    free(exePathCStr);
    
    // Force current working directory to be the Assets folder
    std::string assetsPath = exePath + "/../../Assets/";
    OS_SetCurrentDirectory(assetsPath.c_str());
    
    if(argCount < 2)
    {
        fprintf(stderr, "Insufficient arguments\n");
        return 1;
    }
    
    if(argCount > 2)
    {
        fprintf(stderr, "Too many arguments\n");
        return 1;
    }
    
    const char* modelPath = args[1];
    
    Assimp::Importer importer;
    
    printf("Loading and preprocessing model %s...\n", modelPath);
    
    int flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
        aiProcess_GenUVCoords;
    const aiScene* scene = importer.ReadFile(modelPath, flags);
    
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        fprintf(stderr, "Error loading model: %s\n", importer.GetErrorString());
        return 1;
    }
    
    std::string modelPathNoExt = RemoveFileExtension(modelPath);
    std::string outPath = modelPathNoExt + ".model";
    printf("Writing to %s...\n", outPath.c_str());
    
    FILE* outFile = fopen(outPath.c_str(), "w+b");
    if(!outFile)
    {
        return 1;
    }
    
    defer { fclose(outFile); };
    
    const int version = 0;
    
    Arena arena = ArenaVirtualMemInit(GB(4), MB(2));
    StringBuilder builder = {0};
    defer { FreeBuffers(&builder); };
    UseArena(&builder, &arena);
    
    Append(&builder, "model");
    Put(&builder, (s32)version);
    Put(&builder, (s32)scene->mNumMeshes);
    Put(&builder, (s32)scene->mNumMaterials);
    
    // Write mesh verts and indices
    for(int i = 0; i < scene->mNumMeshes; ++i)
    {
        const aiMesh* mesh = scene->mMeshes[i];
        
        Put(&builder, (s32)mesh->mNumVertices);
        Put(&builder, (s32)(mesh->mNumFaces * 3));
        Put(&builder, (s32)mesh->mMaterialIndex);
        
        Put(&builder, (u8)mesh->HasTextureCoords(0));
        
        for(int j = 0; j < mesh->mNumVertices; ++j)
        {
            Put(&builder, (float)mesh->mVertices[j].x);
            Put(&builder, (float)mesh->mVertices[j].y);
            Put(&builder, (float)mesh->mVertices[j].z);
        }
        for(int j = 0; j < mesh->mNumVertices; ++j)
        {
            Put(&builder, (float)mesh->mNormals[j].x);
            Put(&builder, (float)mesh->mNormals[j].y);
            Put(&builder, (float)mesh->mNormals[j].z);
        }
        
        if(mesh->HasTextureCoords(0))
        {
            for(int j = 0; j < mesh->mNumVertices; ++j)
            {
                Put(&builder, (float)mesh->mTextureCoords[0][j].x);
                Put(&builder, (float)mesh->mTextureCoords[0][j].y);
                Put(&builder, (float)mesh->mTextureCoords[0][j].z);
            }
        }
        
        for(int j = 0; j < mesh->mNumFaces; ++j)
        {
            const aiFace& face = mesh->mFaces[j];
            assert(face.mNumIndices == 3);
            
            Put(&builder, (s32)face.mIndices[0]);
            Put(&builder, (s32)face.mIndices[1]);
            Put(&builder, (s32)face.mIndices[2]);
        }
    }
    
    // Write materials
    for(int i = 0; i < scene->mNumMaterials; ++i)
    {
        const aiMaterial* material = scene->mMaterials[i];
        //WriteMaterial(arena, scene, material, modelNameNoExt);
    }
    
    printf("Success!\n");
    
    WriteToFile(ToString(&builder), outFile);
    
    return 0;
}

// TODO: make functions like these in custom base layer and then use those
std::string RemoveFileExtension(const std::string& fileName)
{
    size_t lastDot = fileName.find_last_of(".");
    if (lastDot == std::string::npos) return fileName;
    return fileName.substr(0, lastDot);
}

// TODO: make functions like these in custom base layer and then use those
std::string RemovePathLastPart(const std::string& fileName)
{
    size_t lastSlash = fileName.find_last_of("/");
    size_t lastBackslash = fileName.find_last_of("\\");
    size_t lastSeparator = std::string::npos;
    
    bool slashFound = lastSlash != std::string::npos;
    bool backslashFound = lastBackslash != std::string::npos;
    
    if(!slashFound && backslashFound)
        lastSeparator = lastBackslash;
    else if(slashFound && !backslashFound)
        lastSeparator = lastSlash;
    else if(!slashFound && !backslashFound)
        lastSeparator = std::string::npos;
    else
        lastSeparator = lastSlash > lastBackslash? lastSlash : lastBackslash;
    
    if (lastSeparator == std::string::npos) return fileName;
    return fileName.substr(0, lastSeparator);
}

// This stuff should all go to a material file
void WriteMaterial(String path, Material mat)
{
    const aiScene* scene = mat.scene;
    const aiMaterial* material = mat.mat;
    
#if 0
    const std::string outTexturesPath = "Textures/";
    
    // Textures
    for(int j = 0; j < ArrayCount(texTypes); ++j)
    {
        int numTextures = material->GetTextureCount(texTypes[j]);
        if(numTextures > 0)
        {
            if(numTextures > 1)
                fprintf(stderr, "This engine's model format currently does not support multiple textures of the same type\n");
            
            aiString path;
            // What to do with this texture mapping?
            aiTextureMapping mapping;
            u32 uvIndex;
            float blendFactor;
            if(material->GetTexture(texTypes[j], 0, &path, &mapping, &uvIndex, &blendFactor) == AI_SUCCESS)
            {
                const aiTexture* texture = scene->GetEmbeddedTexture(path.C_Str());
                if(texture)
                {
                    // Texture is embedded, create a separate texture file
                    // with the name "dst_file_Diffuse.jpg" or similar
                    std::string texStr = aiTextureTypeToString(texTypes[j]);
                    std::string imageName = outTexturesPath + modelName + "_" + texStr + "." + texture->achFormatHint;
                    FILE* image = fopen(imageName.c_str(), "w+b");
                    if(!image)
                    {
                        fprintf(stderr, "Could not write texture file\n");
                        exit(1);
                    }
                    
                    defer { fclose(image); };
                    fwrite(texture->pcData, texture->mWidth, 1, image);
                    
                    // Write the path
                    //Put(&builder, (s32)imageName.length);
                    //Append(&builder, (s32)imageName.c_str());
                }
                else
                {
                    TODO;
                    fprintf(stderr, "Non embedded textures are currently not supported.");
                    exit(1);
                }
            }
        }
        else  // No texture of this type
        {
            //ArenaPushVar<s32>(arena, 0);
        }
    }
    
    if(material->GetTextureCount(aiTextureType_UNKNOWN) > 0)
    {
        fprintf(stderr, "There are textures of unknown type\n");
    }
    
    // Then there's this stuff as well... Properties
    aiColor3D color;
    aiReturn ret;
    ret = material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
    if(ret == aiReturn_SUCCESS) printf("test\n");
    ret = material->Get(AI_MATKEY_COLOR_SPECULAR, color);
    if(ret == aiReturn_SUCCESS) printf("test\n");
    ret = material->Get(AI_MATKEY_COLOR_AMBIENT, color);
    if(ret == aiReturn_SUCCESS) printf("test\n");
    ret = material->Get(AI_MATKEY_COLOR_EMISSIVE, color);
    if(ret == aiReturn_SUCCESS) printf("test\n");
    ret = material->Get(AI_MATKEY_COLOR_TRANSPARENT, color);
    if(ret == aiReturn_SUCCESS) printf("test\n");
#endif
}

void WriteTexture()
{
    
}