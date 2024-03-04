
#pragma once

#include "os/os_generic.h"
#include "renderer/renderer_generic.h"

struct RenderSettings;

struct Particle
{
    Vec3 position;
    Vec3 color;
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
