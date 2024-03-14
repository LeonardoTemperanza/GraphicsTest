
// NOTE: This is very much a work in progress.
// This system should have its own arena, which gets freed
// when loading a new scene.
// Error handling will all be revisited once we have a working gui.
// (a linked list of errors will be maintained and then outputted in the gui
// in text form.

// It seems that a lot of custom engines have a "tag" system, where:
// There's a file that has all tag->mappings, like "player : Textures/Player/player.png"
// Then the code just uses the tag to use the texture (in non-release builds for hot 
// reloading, otherwise it's just compiled as a constant)
// Even when we're talking scenes, maybe this would still work, i guess you won't have a
// single table but i'm sure you can do something similar to this. Even the best release mode 
// thing would still have a hash table lookup, but instead of looking up a string an integer 
// is looked up. You might even be able to just have a perfect hash table

// Does this mean that i have to add some stuff to this every time i want a new "asset",
// or virtual asset i guess

enum AssetTag
{
    PlayerModel,
    //
};

#pragma once

#include "base.h"

struct Material
{
    int testt;
};

struct Mesh
{
    bool hasTextureCoords;
    
    Slice<Vec3> verts;
    Slice<Vec3> textureCoords;
    Slice<s32>  indices;
};

struct Model
{
    Slice<Mesh>     meshes;
    Slice<Material> material;
};

Model* LoadModel(const char* path, Arena* dst);
