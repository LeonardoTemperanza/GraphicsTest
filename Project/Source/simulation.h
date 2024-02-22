
#pragma once

#include "os/os_generic.h"
#include "renderer/renderer_generic.h"

struct RenderSettings;

struct Particle
{
    Vec3 position;
    Vec3 color;
};

struct InputDominator
{
    // Input stuff which is relevant
    // in between frames. Default is 0 for both,
    // which means the application starts with having
    // no controller active, and an idx of 0. Controller 0
    // (or any inactive controller) can still be accessed,
    // no problem. It will have null state anyway, so no errors
    // or unintended side effects.
    int idx;
    bool active;
};

struct AppState
{
    InputDominator domGamepad;
    InputDominator domKeyboard;
    
    RenderSettings renderSettings;
};

AppState InitSimulation();

// Input management
bool IsGamepadStateNull(GamepadState* gamepads, int idx);
InputDominator GetDominatingGamepad(InputState input, InputDominator prevDom);
InputDominator GetDominatingKeyboard(InputState input, InputDominator prevDom);

// Main simulation
void MainUpdate(AppState* state, float deltaTime, InputState input, Arena* permArena, Arena* frameArena);
void UpdateCamera(float deltaTime, GamepadState* gamepad);
