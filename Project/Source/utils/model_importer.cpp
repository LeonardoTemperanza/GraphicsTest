
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "os/os_generic.cpp"
#include "base.cpp"
#include "asset_system.h"

#include <iostream>

const char* defaultTexturePath = "Default/white.png";

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
struct PathName
{
    // Path relative to Assets folder
    // if nameLen is 0 it means that it doesn't have this
    // texture type
    s32 nameLen;
    char name[nameLen (which could be 0)];
};

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
        Vertex_v0 verts[numVerts];  // From asset_system.h
        s32 indices[numIndices];  // Grouped in 3 to form a triangle
    };
    
    Mesh meshes[numMeshes];
};

struct Material
{
    s32 numTextures;
    PathName textures[numTextures];
    //PathName shader;
};

struct Shader
{
    // Some info on the shader here...
    s32 numTextures;
    
};

// Version 1...

#endif

std::string RemoveFileExtension(const std::string& fileName);
std::string RemovePathLastPart(const std::string& fileName);

bool WriteMaterial(const char* modelPath, int materialIdx, const char* path, const aiScene* scene, const aiMaterial* material);

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
    
    printf("Running version %d of the model importer.\n", 0);
    fflush(stdout);
    
    const char* modelPath = args[1];
    
    Assimp::Importer importer;
    
    printf("Loading and preprocessing model %s...\n", modelPath);
    fflush(stdout);
    
    int flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | 
        aiProcess_GenUVCoords | aiProcess_MakeLeftHanded | aiProcess_FlipUVs | aiProcess_GlobalScale | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace;
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
        fprintf(stderr, "Error writing to file %s.\n", outPath.c_str());
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
    
#if 0
    // Write material paths
    
    for(int i = 0; i < scene->mNumMaterials; ++i)
    {
        // Decide the path, write the material file
        std::string materialStr = "_material";
        materialStr += std::to_string(i);
        std::string materialPath = modelPathNoExt + materialStr + ".mat";
        bool ok = WriteMaterial(modelPath, i, materialPath.c_str(), scene, scene->mMaterials[i]);
        if(!ok) return 1;
        
        Put(&builder, (s32)materialPath.size());
        Append(&builder, materialPath.c_str());
    }
#endif
    
    aiNode* root = scene->mRootNode;
    
    for(int i = 0; i < scene->mNumMeshes; ++i)
    {
        const aiMesh* mesh = scene->mMeshes[i];
        
        Put(&builder, (s32)mesh->mNumVertices);
        Put(&builder, (s32)(mesh->mNumFaces * 3));
        Put(&builder, (s32)mesh->mMaterialIndex);
        Put(&builder, (u8)mesh->HasTextureCoords(0));
        
        for(int j = 0; j < mesh->mNumVertices; ++j)
        {
            Vertex_v0 vert = {0};
            vert.pos.x = mesh->mVertices[j].x;
            vert.pos.y = mesh->mVertices[j].y;
            vert.pos.z = mesh->mVertices[j].z;
            vert.normal.x = mesh->mNormals[j].x;
            vert.normal.y = mesh->mNormals[j].y;
            vert.normal.z = mesh->mNormals[j].z;
            
            if(mesh->HasTextureCoords(0))
            {
                vert.texCoord.x = mesh->mTextureCoords[0][j].x;
                vert.texCoord.y = mesh->mTextureCoords[0][j].y;
            }
            
            vert.tangent.x = mesh->mTangents[j].x;
            vert.tangent.y = mesh->mTangents[j].y;
            vert.tangent.z = mesh->mTangents[j].z;
            // Bitangent is derived in the vertex shader
            // from normals and tangents
            //vert.bitangent.x = mesh->mBitangents[j].x;
            //vert.bitangent.y = mesh->mBitangents[j].y;
            //vert.bitangent.z = mesh->mBitangents[j].z;
            
            Put(&builder, vert);
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
    
    printf("Writing to file...\n");
    WriteToFile(ToString(&builder), outFile);
    printf("Success!\n");
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
bool WriteMaterial(const char* modelPath, int materialIdx, const char* path, const aiScene* scene, const aiMaterial* material)
{
    ScratchArena scratch;
    
    FILE* matFile = fopen(path, "w+b");
    if(!matFile)
    {
        fprintf(stderr, "Error: Could not write to file.\n");
        return false;
    }
    
    defer { fclose(matFile); };
    
    int shadingModel = 0;
    material->Get(AI_MATKEY_SHADING_MODEL, shadingModel);
    
    const char* shadingModeStr = nullptr;
    switch(shadingModel)
    {
        case aiShadingMode_Phong:
        {
            shadingModeStr = "Phong";
            break;
        }
        case aiShadingMode_PBR_BRDF:
        {
            shadingModeStr = "PBR BRDF";
            break;
        }
    }
    
    if(shadingModeStr)
        printf("Shading mode: %s\n", shadingModeStr);
    else
        fprintf(stderr, "Material %d of model %s uses unknown shading mode.\n", materialIdx, modelPath);
    
    StringBuilder builder = {0};
    UseArena(&builder, scratch);
    
    s32 numTextures = ArrayCount(texTypes);
    Append(&builder, "material");
    Put(&builder, (s32)numTextures);
    
    // Textures
    for(int j = 0; j < numTextures; ++j)
    {
        int count = material->GetTextureCount(texTypes[j]);
        if(count > 0)
        {
            if(count > 1)
            {
                fprintf(stderr, "This engine's model format currently does not support multiple textures of the same type. The ones following the first will be ignored.\n");
            }
            
            aiString texPath;
            // What to do with this texture mapping?
            aiTextureMapping mapping;
            u32 uvIndex;
            float blendFactor;
            if(material->GetTexture(texTypes[j], 0, &texPath, &mapping, &uvIndex, &blendFactor) == AI_SUCCESS)
            {
                const aiTexture* texture = scene->GetEmbeddedTexture(texPath.C_Str());
                
                // Create a separate texture file
                // with the name "dst_file_Diffuse.jpg" or similar
                std::string texStr = aiTextureTypeToString(texTypes[j]);
                printf("Found texture type: %s.\n", texStr.c_str());
                
                if(texture)
                {
                    std::string outPath = RemoveFileExtension(path) + "_" + texStr + "." + texture->achFormatHint;
                    FILE* image = fopen(outPath.c_str(), "w+b");
                    if(!image)
                    {
                        fprintf(stderr, "Could not write texture file\n");
                        return false;
                    }
                    
                    defer { fclose(image); };
                    fwrite(texture->pcData, texture->mWidth, 1, image);
                    
                    // Write the path
                    Put(&builder, (s32)outPath.size());
                    Append(&builder, outPath.c_str());
                }
                else
                {
                    // Not embedded, just use the default texture for now
                    fprintf(stderr, "Material %d of model %s has a non-embedded texture of type '%s'. Non-embedded textures are temporarily not supported. This has been set to the default texture ('%s').\n", materialIdx, modelPath, aiTextureTypeToString(texTypes[j]), "phong", defaultTexturePath);
                    
                    Put(&builder, (s32)0);
                    
                    //Put(&builder, (s32)strlen(defaultTexturePath));
                    //Append(&builder, defaultTexturePath);
                }
            }
        }
        else  // No texture of this type
        {
            fprintf(stderr, "Material %d of model %s does not contain the texture type '%s', which is needed by the '%s' shader. This has been set to the default texture ('%s').\n", materialIdx, modelPath, aiTextureTypeToString(texTypes[j]), "phong", defaultTexturePath);
            
            Put(&builder, (s32)0);
            //Put(&builder, (s32)strlen(defaultTexturePath));
            //Append(&builder, defaultTexturePath);
        }
    }
    
    if(material->GetTextureCount(aiTextureType_UNKNOWN) > 0)
        fprintf(stderr, "There are textures of unknown type, they will be ignored.\n");
    
    // Then there's this stuff as well... Properties. Not sure if it's useful to support this stuff or not.
#if 0
    aiColor3D color;
    aiReturn ret;
    ret = material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
    if(ret == aiReturn_SUCCESS) printf("Diffuse color defined!\n");
    ret = material->Get(AI_MATKEY_COLOR_SPECULAR, color);
    if(ret == aiReturn_SUCCESS) printf("Specular color defined!\n");
    ret = material->Get(AI_MATKEY_COLOR_AMBIENT, color);
    if(ret == aiReturn_SUCCESS) printf("Ambient color defined!\n");
    ret = material->Get(AI_MATKEY_COLOR_EMISSIVE, color);
    if(ret == aiReturn_SUCCESS) printf("Emissive color defined!\n");
    ret = material->Get(AI_MATKEY_COLOR_TRANSPARENT, color);
    if(ret == aiReturn_SUCCESS) printf("Transparent color defined!\n");
#endif
    
    WriteToFile(ToString(&builder), matFile);
    return true;
}
