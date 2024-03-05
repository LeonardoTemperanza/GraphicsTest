
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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

// The following specification describes the
// general data layout of the file
#if 0
struct Model
{
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
    
    struct Material
    {
        struct Texture
        {
            // Path to the image (from Assets directory)
            // This will probably be different
            // when we move to a package system
            s32 pathLen;
            char path[pathLen];
        };
        
        // Array of textures following 'texTypes'
        Texture textures[..];
    };
    
    Material materials[numMaterials];
};

// Version 1...

#endif

void LoadModel();
void LoadTexture(s32 version);

int main(int argCount, char** args)
{
    if(argCount < 3)
    {
        fprintf(stderr, "Insufficient arguments\n");
        return 1;
    }
    
    if(argCount > 3)
    {
        fprintf(stderr, "Too many arguments\n");
        return 1;
    }
    
    const char* inPath  = args[1];
    const char* outPath = args[2];
    
    Assimp::Importer importer;
    
    const aiScene* scene = importer.ReadFile(inPath, aiProcess_Triangulate);
    
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        fprintf(stderr, "Error loading model: %s\n", importer.GetErrorString());
        return 1;
    }
    
    FILE* outFile = fopen(outPath, "wb");
    defer { fclose(outFile); };
    if(!outFile)
    {
        fprintf(stderr, "Could not write to file.\n");
        return 1;
    }
    
    const int version = 0;
    
    Arena a = ArenaVirtualMemInit(GB(4), MB(2));
    Arena* arena = &a;
    
    ArenaPushVar<s32>(arena, version);
    ArenaPushVar<s32>(arena, scene->mNumMeshes);
    ArenaPushVar<s32>(arena, scene->mNumMaterials);
    
    // Write mesh verts and indices
    for(int i = 0; i < scene->mNumMeshes; ++i)
    {
        const aiMesh* mesh = scene->mMeshes[i];
        
        ArenaPushVar<s32>(arena, mesh->mNumVertices);
        ArenaPushVar<s32>(arena, mesh->mNumFaces * 3);
        ArenaPushVar<s32>(arena, mesh->mMaterialIndex);
        ArenaPushVar<bool>(arena, mesh->HasTextureCoords(0));
        
        for(int j = 0; j < mesh->mNumVertices; ++j)
        {
            ArenaPushVar<float>(arena, mesh->mVertices[j].x);
            ArenaPushVar<float>(arena, mesh->mVertices[j].y);
            ArenaPushVar<float>(arena, mesh->mVertices[j].z);
        }
        for(int j = 0; j < mesh->mNumVertices; ++j)
        {
            ArenaPushVar<float>(arena, mesh->mNormals[j].x);
            ArenaPushVar<float>(arena, mesh->mNormals[j].y);
            ArenaPushVar<float>(arena, mesh->mNormals[j].z);
        }
        
        if(mesh->HasTextureCoords(0))
        {
            for(int j = 0; j < mesh->mNumVertices; ++j)
            {
                ArenaPushVar<float>(arena, mesh->mTextureCoords[0][j].x);
                ArenaPushVar<float>(arena, mesh->mTextureCoords[0][j].y);
                ArenaPushVar<float>(arena, mesh->mTextureCoords[0][j].z);
            }
        }
        
        for(int j = 0; j < mesh->mNumFaces; ++j)
        {
            const aiFace& face = mesh->mFaces[i];
            assert(face.mNumIndices == 3);
            
            ArenaPushVar<s32>(arena, face.mIndices[0]);
            ArenaPushVar<s32>(arena, face.mIndices[1]);
            ArenaPushVar<s32>(arena, face.mIndices[2]);
        }
    }
    
    // Write materials
    for(int i = 0; i < scene->mNumMaterials; ++i)
    {
        const aiMaterial* material = scene->mMaterials[i];
        
        for(int j = 0; j < ArrayCount(texTypes); ++j)
        {
            int numTextures = material->GetTextureCount(texTypes[j]);
            
            if(numTextures > 0)
            {
                if(numTextures > 1)
                    fprintf(stderr, "This engine's model format currently does not support multiple textures of the same type\n");
                
                printf("Material has %d\n", j);
                
                aiString path;
                aiTextureMapping mapping;
                u32 uvIndex;
                float blendFactor;
                if(material->GetTexture(texTypes[j], 0, &path, &mapping, &uvIndex, &blendFactor) == AI_SUCCESS)
                {
                    const aiTexture* texture = scene->GetEmbeddedTexture(path.C_Str());
                    if(texture)
                    {
                        texture->achFormatHint;
                        texture->mWidth;
                        texture->pcData;
                    }
                    else
                    {
                        path;
                    }
                }
            }
        }
        
        if(material->GetTextureCount(aiTextureType_UNKNOWN) > 0)
        {
            fprintf(stderr, "There are textures of unknown type\n");
        }
    }
    
    // Print info about each mesh in the model
    for(int i = 0; i < scene->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[i];
        printf("Mesh %d:\n", i+1);
        printf("    Verts: %d\n", mesh->mNumVertices);
        printf("    Faces: %d\n", mesh->mNumFaces);
        
        if(i < scene->mNumMeshes-1) printf("\n");
    }
    
    LoadModel();
    
    return 0;
}

void LoadModel()
{
    int version = 0;
    printf("Version: %d\n", version);
    
    // Diffuse
    LoadTexture(version);
    
    // Normal
    LoadTexture(version);
}

void LoadTexture(s32 version)
{
    
}
