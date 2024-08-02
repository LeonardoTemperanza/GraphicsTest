
#pragma once

#include "base.h"

struct GamepadState
{
    u32 buttons;
    
    // Normalized form 0 to 1
    float leftTrigger;
    float rightTrigger;
    
    // Values are normalized from -1 to 1.
    Vec2 leftStick;
    Vec2 rightStick;
};

struct Input
{
    bool changedDom;
    
    GamepadState gamepad;
    
    // From OS layer
    bool virtualKeys[Keycode_Count];
    bool unfilteredKeys[Keycode_Count];
    
    int mouseX;
    int mouseY;
    Vec2 mouseDelta;
    
    // Prev input state (names are
    // repeated for simplicity of API
    // and ease of access, since in C++
    // you can't really have implicit
    // struct members)
    struct
    {
        GamepadState gamepad;
        bool virtualKeys[Keycode_Count];
        bool unfilteredKeys[Keycode_Count];
        s64 mouseX;
        s64 mouseY;
        Vec2 mouseDelta;
    } prev;
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

struct InputCtx
{
    // Has the first input poll been performed?
    bool followingPolls;
    Input curInput;
    
    // Used for setting the mouse position
    // at the next poll call.
    s64 deferredMousePosX;
    s64 deferredMousePosY;
    bool setMousePos;
    
    // Currently dominating gamepad
    InputDominator gamepadDom;
    
    // This could have all gamepad mappings
    // and stuff like that in the future,
    // the deadzone values, etc.
};

void PollAndProcessInput();
Input GetInput();
void SetMousePos(s64 mouseX, s64 mouseY);

// Internal procs
void ApplyDeadzones(OS_GamepadState* gamepadState);
bool IsGamepadStateNull(OS_GamepadState gamepadState);
InputDominator FindDominatingGamepad(OS_InputState input, InputDominator prevDom);
bool PressedKey(Input input, VirtualKeycode key);
bool PressedUnfilteredKey(Input input, VirtualKeycode key);