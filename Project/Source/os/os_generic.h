
// TODO: Provide mouse delta as well. Also provide
// functionality for "lock the mouse in place and make it disappear"
// and revert that. Having this high level functionality in the OS
// layer would be better, i think, because OSs do all sorts of stuff with
// window focus and all that. And it's also i think the only instance where i'd
// ever want to hide the mouse anyway

// This implements platform behavior for the
// specific application we're developing

#pragma once

#include "base.h"

// Application management

struct OS_FileSystemChange
{
    const char* file;
};

void OS_Init(const char* windowName);
void OS_ShowWindow();
void OS_GetWindowSize(int* width, int* height);
void OS_GetClientAreaSize(int* width, int* height);
// Return value = false means to quit the application,
// true means the opposite 
bool OS_HandleWindowEvents();
// For simplicity we assume that a single file watcher on a single directory
// is active at any given time.
void OS_StartFileWatcher(const char* path);
void OS_StopFileWatcher();
Slice<OS_FileSystemChange> OS_ConsumeFileWatcherChanges(Arena* dst);
void OS_Cleanup();

// Opengl (some platforms have tight coupling with opengl)
#ifdef GFX_OPENGL
void OS_OpenglSwapBuffers();
#endif

// Win32 (rendering api initialization might need explicit access to the window handle)
#ifdef _WIN32
void* Win32_GetWindowHandle();
#endif

// Input

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
    Keycode_Insert,
    Keycode_Ctrl,
    Keycode_Space,
    Keycode_Esc,
    Keycode_Del,
    Keycode_Count // This is just for the size of enum
};

struct OS_MouseState
{
    bool active; // On the window?
    
    // In pixels starting from the top-left corner
    // of the application window. This is guaranteed
    // to be < 0 if the cursor is not on the window
    int xPos;
    int yPos;
    float deltaX;
    float deltaY;
};

// All inactive gamepads will have every property set
// to 0, so no need to check for the active flag unless needed.
struct OS_GamepadState
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
struct OS_InputState
{
    OS_GamepadState gamepads[MaxActiveControllers];
    OS_MouseState mouse;
    
    // This includes mouse buttons
    bool virtualKeys[Keycode_Count];
};

OS_InputState OS_PollInput();

void OS_ShowCursor(bool show);
void OS_FixCursor(bool fix);  // The mouse delta should still be provided even after fixing the cursor

// Sound

// Misc
void OS_FatalError(const char* message);
uint64_t OS_GetTicks();
double OS_GetElapsedSeconds(uint64_t startTicks, uint64_t endTicks);
float OS_GetDPIScale();

// Libraries
void OS_InitDearImgui();
void OS_DearImguiBeginFrame();
