
#pragma once

#include "os/os_generic.h"
#include "renderer/renderer_generic.h"

struct RenderSettings;

struct Entity
{
    Vec3 pos;
    Quat rot;
    Vec3 scale;
    
    u32 flags;
    u16 key;
    u16 gen;
    
    Model* model;
    
    // There will be many more of course
    
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
