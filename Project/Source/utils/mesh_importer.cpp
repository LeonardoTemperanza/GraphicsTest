
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "base.cpp"
#include "serialization.h"

#include <iostream>

const char* defaultTexturePath = "Default/white.png";

// Model file format
bool WriteMaterial(const char* modelPath, int materialIdx, const char* path, const aiScene* scene, const aiMaterial* material);

// Usage:
// model_importer.exe file_to_import.(obj/fbx/...)
// The path is relative to the Assets folder
int main(int argCount, char** args)
{
    ScratchArena scratch;
    
    char* exePathCStr = GetExecutablePath();
    defer { free(exePathCStr); };
    String exePath = {.ptr = exePathCStr, .len = (s64)strlen(exePathCStr)};
    exePath = PopLastDirFromPath(exePath);
    
    // Force current working directory to be the Assets folder.
    // Currently in Project/Build/utils
    {
        StringBuilder builder = {};
        UseArena(&builder, scratch);
        Append(&builder, exePath);
        Append(&builder, "/../../../Assets/");
        NullTerminate(&builder);
        B_SetCurrentDirectory(ToString(&builder).ptr);
    }
    
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
    
    int flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals
        | aiProcess_GenUVCoords | aiProcess_MakeLeftHanded | aiProcess_FlipUVs | aiProcess_GlobalScale
        | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace;
    
    const aiScene* scene = importer.ReadFile(modelPath, flags);
    
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        fprintf(stderr, "Error loading model: %s\n", importer.GetErrorString());
        return 1;
    }
    
    for(int i = 0; i < scene->mNumMeshes; ++i)
    {
        const aiMesh* mesh = scene->mMeshes[i];
        
        // @tmp Buffer overflow, though it's practically impossible
        char str[100];
        itoa(i+1, str, 10);
        const char* meshIdxStr = (const char*)&str;
        
        String modelPathNoExt = GetPathNoExtension(modelPath);
        StringBuilder pathBuilder = {0};
        UseArena(&pathBuilder, scratch);
        Append(&pathBuilder, modelPathNoExt);
        
        if(scene->mNumMeshes > 1)
        {
            Append(&pathBuilder, "_");
            Append(&pathBuilder, meshIdxStr);
        }
        
        Append(&pathBuilder, ".mesh");
        NullTerminate(&pathBuilder);
        const char* outPath = ToString(&pathBuilder).ptr;
        
        FILE* outFile = fopen(outPath, "w+b");
        if(!outFile)
        {
            fprintf(stderr, "Error writing to file %s.\n", outPath);
            return 1;
        }
        
        defer { fclose(outFile); };
        
        Arena arena = ArenaVirtualMemInit(GB(4), MB(2));
        StringBuilder binary = {0};
        UseArena(&binary, &arena);
        
        // NOTE: Change whenever version changes
        const int version = 0;
        
        Append(&binary, "mesh");
        Put(&binary, (u32)version);
        
        MeshHeader_v0 header = {};
        header.isSkinned = false;
        header.numVerts = mesh->mNumVertices;
        header.numIndices = mesh->mNumFaces * 3;
        header.hasTextureCoords = true;
        header.vertsOffset = sizeof(MeshHeader_v0);
        header.indicesOffset = header.vertsOffset + sizeof(Vertex) * mesh->mNumVertices;
        
        Put(&binary, header);
        
        for(int j = 0; j < mesh->mNumVertices; ++j)
        {
            Vertex vert = {0};
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
            
            Put(&binary, vert);
        }
        
        for(int j = 0; j < mesh->mNumFaces; ++j)
        {
            const aiFace& face = mesh->mFaces[j];
            assert(face.mNumIndices == 3);
            
            Put(&binary, (s32)face.mIndices[0]);
            Put(&binary, (s32)face.mIndices[1]);
            Put(&binary, (s32)face.mIndices[2]);
        }
        
        WriteToFile(ToString(&binary), outFile);
        printf("Successfully imported to '%s'\n", outPath);
    }
    
    return 0;
}
