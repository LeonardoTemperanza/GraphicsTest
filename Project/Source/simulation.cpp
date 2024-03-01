
#include "os/os_generic.h"
#include "simulation.h"

// TODO: there should be a separate file
// that deals with input stuff. There should
// be a clean API that just lets you:
// 1) poll input (and process it, apply )
// 2) get input (access the current input state with a function)
// 3) the input should include: this and last frame's input, as well
//    as mouse movement deltas

// Deadzones
const float stickDeadzone = 0.1f;
const float triggerDeadzone = 0.1f;

AppState InitSimulation()
{
    AppState state = {0};
    state.renderSettings.camera.position.z = -10.0f;
    state.renderSettings.camera.rotation = Quat::identity;
    return state;
}

bool IsGamepadStateNull(GamepadState* gamepads, int idx)
{
    bool null = true;
    GamepadState& gamepad = gamepads[idx];
    null &= gamepad.buttons == 0;
    null &= abs(gamepad.leftTrigger)  < triggerDeadzone;
    null &= abs(gamepad.rightTrigger) < triggerDeadzone;
    null &= abs(gamepad.leftStickX)   < stickDeadzone;
    null &= abs(gamepad.leftStickY)   < stickDeadzone;
    null &= abs(gamepad.rightStickX)  < stickDeadzone;
    null &= abs(gamepad.rightStickY)  < stickDeadzone;
    return !gamepads[idx].active || null;
}

InputDominator GetDominatingGamepad(InputState input, InputDominator prevDom)
{
    // By default return the previous one
    InputDominator res = prevDom;
    
    // Find out if its input is null this frame.
    // If it is, it could potentially be dominated
    // by some other gamepad.
    bool gamepadStateNull = IsGamepadStateNull(input.gamepads, prevDom.idx);
    bool checkForDominator = !prevDom.active || (prevDom.active && gamepadStateNull);
    if(checkForDominator)
    {
        for(int i = 0; i < MaxActiveControllers; ++i)
        {
            if(prevDom.active && i == prevDom.idx) continue;
            
            if(!IsGamepadStateNull(input.gamepads, i))
            {
                res.active = true;
                res.idx = i;
                break;
            }
        }
    }
    
    return res;
}

void ApplyDeadzone(InputState* input)
{
    for(int i = 0; i < MaxActiveControllers; ++i)
    {
        GamepadState& gamepad = input->gamepads[i];
        if(abs(gamepad.leftTrigger)  < triggerDeadzone) gamepad.leftTrigger  = 0;
        if(abs(gamepad.rightTrigger) < triggerDeadzone) gamepad.rightTrigger = 0;
        if(abs(gamepad.leftStickX)   < stickDeadzone)   gamepad.leftStickX   = 0;
        if(abs(gamepad.leftStickY)   < stickDeadzone)   gamepad.leftStickY   = 0;
        if(abs(gamepad.rightStickX)  < stickDeadzone)   gamepad.rightStickX  = 0;
        if(abs(gamepad.rightStickY)  < stickDeadzone)   gamepad.rightStickY  = 0;
    }
}

InputDominator GetDominatingKeyboard(InputState input, InputDominator prevDom)
{
    TODO;
    return prevDom;
}

void UpdateCamera(Transform* camera, float deltaTime, GamepadState* gamepad)
{
    // Camera rotation
    const float rotateXSpeed = Deg2Rad(120);
    const float rotateYSpeed = Deg2Rad(80);
    static float angleX = 0.0f;
    static float angleY = 0.0f;
    
    angleX += rotateXSpeed * gamepad->rightStickX * deltaTime;
    angleY += rotateYSpeed * gamepad->rightStickY * deltaTime;
    while(angleX < 0.0f) angleX += 2*Pi;
    while(angleX > 2*Pi) angleX -= 2*Pi;
    
    angleY = clamp(angleY, Deg2Rad(-90), Deg2Rad(90));
    
    Quat yRot = AngleAxis(Vec3::left, angleY);
    Quat xRot = AngleAxis(Vec3::up, angleX);
    camera->rotation = xRot * yRot;
    
    // Camera position
    static Vec3 curVel = {0};
    
    const float moveSpeed = 4.0f;
    const float moveAccel = 30.0f;
    Vec3 targetVel =
    {
        .x = gamepad->leftStickX * moveSpeed,
        .y = 0.0f,
        .z = gamepad->leftStickY * moveSpeed
    };
    
    if(dot(targetVel, targetVel) > moveSpeed * moveSpeed)
        targetVel = normalize(targetVel) * moveSpeed;
    
    targetVel = camera->rotation * targetVel;
    targetVel.y += (gamepad->rightTrigger - gamepad->leftTrigger) * moveSpeed;
    
    curVel = MoveTowards(curVel, targetVel, moveAccel * deltaTime);
    camera->position += curVel * deltaTime;
}

void UpdateUI(InputState input)
{
    // Example of how the UI might work
#if 0
    UI_BeginFrame(input);
    
    auto window = UI_Container();
    UI_PushParent(window);
    
    if(UI_Button("Hello").clicked)
    {
        
    }
    
    UI_PopParent(window);
    UI_EndFrame();
#endif
}

void MainUpdate(AppState* state, float deltaTime, InputState input, Arena* permArena, Arena* frameArena)
{
    ApplyDeadzone(&input);
    
    // Disambiguate between different devices
    GamepadState* gamepad = nullptr;
    bool gamepadChanged = false;
    {
        InputDominator domGamepad = GetDominatingGamepad(input, state->domGamepad);
        
        // This could be useful for displaying some kind of messages
        gamepadChanged = state->domGamepad.idx != domGamepad.idx && state->domGamepad.active;
        
        state->domGamepad = domGamepad;
        
        // Currently used gamepad
        gamepad = &input.gamepads[domGamepad.idx];
    }
    
    UpdateUI(input);
    
    static bool lastRMouse = false;
    if(input.virtualKeys[Keycode_RMouse])
    {
        if(!lastRMouse)
        {
            state->lockMousePosX = input.mouse.xPos;
            state->lockMousePosY = input.mouse.yPos;
        }
        
        OS_ShowCursor(false);
        OS_SetCursorPos(state->lockMousePosX, state->lockMousePosY);
        lastRMouse = true;
    }
    else
    {
        OS_ShowCursor(true);
        lastRMouse = false;
    }
    
    UpdateCamera(&state->renderSettings.camera, deltaTime, gamepad);
    
    // Update simulation state
    
    // Render settings
    state->renderSettings.horizontalFOV = Deg2Rad(90);
    state->renderSettings.nearClipPlane = 0.5f;
    state->renderSettings.farClipPlane  = 1000.0f;
}
