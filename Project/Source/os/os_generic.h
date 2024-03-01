
#pragma once

#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <cstdint>

#include "base.h"

enum OS_GraphicsLib
{
    GfxLib_None = 0,
    GfxLib_OpenGL,
    GfxLib_D3D11
};

enum GamepadButtonField
{
    Gamepad_None           = 0,
    Gamepad_Dpad_Up        = 1 << 1,
    Gamepad_Dpad_Down      = 1 << 2,
    Gamepad_Dpad_Left      = 1 << 3,
    Gamepad_Dpad_Right     = 1 << 4,
    Gamepad_Start          = 1 << 5,
    Gamepad_Back           = 1 << 6,
    Gamepad_LeftThumb      = 1 << 7,
    Gamepad_RightThumb     = 1 << 8,
    Gamepad_LeftShoulder   = 1 << 9,
    Gamepad_RightShoulder  = 1 << 10,
    Gamepad_A              = 1 << 11,
    Gamepad_B              = 1 << 12,
    Gamepad_X              = 1 << 13,
    Gamepad_Y              = 1 << 14
};

enum VirtualKeycode
{
    Keycode_Null = 0,
    Keycode_LMouse,
    Keycode_RMouse,
    Keycode_MMouse,
    Keycode_A,
    Keycode_B,
    Keycode_C,
    Keycode_D,
    Keycode_E,
    Keycode_F,
    Keycode_G,
    Keycode_H,
    Keycode_I,
    Keycode_J,
    Keycode_K,
    Keycode_L,
    Keycode_M,
    Keycode_N,
    Keycode_O,
    Keycode_P,
    Keycode_Q,
    Keycode_R,
    Keycode_S,
    Keycode_T,
    Keycode_U,
    Keycode_V,
    Keycode_W,
    Keycode_X,
    Keycode_Y,
    Keycode_Z,
    Keycode_Count // This is just for the size of enum
};

struct MouseState
{
    bool active; // On the window?
    
    // In pixels starting from the top-left corner
    // of the application window. This is guaranteed
    // to be < 0 if the cursor is not on the window
    int64_t xPos;
    int64_t yPos;
};

// All inactive gamepads will have every property set
// to 0, so no need to check for the active flag unless needed.
struct GamepadState
{
    bool active;
    uint32_t buttons;
    
    // Values are normalized from 0 to 1.
    float leftTrigger;
    float rightTrigger;
    
    // Values are normalized from -1 to 1.
    float leftStickX;
    float leftStickY;
    float rightStickX;
    float rightStickY;
};

#define MaxActiveControllers 8
struct InputState
{
    GamepadState gamepads[MaxActiveControllers];
    MouseState mouse;
    
    // This includes mouse buttons
    bool virtualKeys[Keycode_Count];
};

// Application management
OS_GraphicsLib OS_Init();
void OS_DebugMessage(const char* message);
void OS_FatalError(const char* message);
bool OS_IsDebuggerPresent();
void OS_ShowWindow();
void OS_GetWindowSize(int* width, int* height);
void OS_GetClientAreaSize(int* width, int* height);
void OS_SwapBuffers();
// Return value represents whether or not a quit
// message has been received
bool OS_HandleWindowEvents();
void OS_Cleanup();

// Memory
void* OS_MemReserve(uint64_t size);
void OS_MemCommit(void* mem, uint64_t size);
void OS_MemFree(void* mem, uint64_t size);

// Input
InputState OS_PollInput();

// Sound

// Miscelleaneous
void OS_Sleep(uint64_t millis);
uint64_t OS_GetTicks();
double OS_GetElapsedSeconds(uint64_t startTicks, uint64_t endTicks);

void OS_ShowCursor(bool show);
// Position should be supplied as window relative
void OS_SetCursorPos(int64_t posX, int64_t posY);