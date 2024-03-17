
#pragma once

#include "os/os_generic.h"
#include "renderer/renderer_generic.h"

struct RenderSettings;

enum EntityFlags
{
    EntityFlags_Dirty       = 1 << 0,
    EntityFlags_Destroyed   = 1 << 1,
    EntityFlags_Transparent = 1 << 2
};

struct EntityKey
{
    u16 id;
    u16 gen;
};

struct Model;
struct Entity
{
    Vec3 pos;
    Quat rot;
    Vec3 scale;
    
    u32 flags;
    EntityKey key;
    
    Model* model;  // Can be nullptr for lights, sound effects, etc.
    
    u16 mountEntityId;
    u16 mountBoneId;
};

struct Door
{
    Entity* base;
};

struct AppState
{
    // If locking the mouse position,
    // this is the value to set it to
    s64 lockMousePosX;
    s64 lockMousePosY;
    
    Slice<Entity> entities;
    RenderSettings renderSettings;
};

// Main simulation
AppState InitSimulation();
void MainUpdate(AppState* state, float deltaTime, Arena* permArena, Arena* frameArena);
void MainRender(AppState* state, RenderSettings settings);
void UpdateCamera(Transform* camera, float deltaTime);
