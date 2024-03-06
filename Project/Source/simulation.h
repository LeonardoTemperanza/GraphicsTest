
#pragma once

#include "os/os_generic.h"
#include "renderer/renderer_generic.h"

struct RenderSettings;

// I'm thinking all the assets (models, audio)
// are loaded via key mechanism and the entity
// just refers to that key for its model.

struct Particle
{
    Vec3 position;
    Vec3 color;
};

struct Entity
{
    u16 flags;
    u16 gen;
    u32 key;
    
    Transform transform;
    
    //AKey model;
};

struct AppState
{
    // If locking the mouse position,
    // this is the value to set it to
    s64 lockMousePosX;
    s64 lockMousePosY;
    
    RenderSettings renderSettings;
};

AppState InitSimulation();

// Main simulation
void MainUpdate(AppState* state, float deltaTime, Arena* permArena, Arena* frameArena);
void UpdateCamera(Transform* camera, float deltaTime);
