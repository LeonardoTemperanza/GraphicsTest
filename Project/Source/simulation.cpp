
#include "os/os_generic.h"
#include "simulation.h"

// TODO: there should be a separate file
// that deals with input stuff. There should
// be a clean API that just lets you:
// 1) poll input (and process it, apply deadzones)
// 2) get input (access the current input state with a function)
// 3) the input should include: this and last frame's input, as well
//    as mouse movement deltas

// Deadzones
//const float stickDeadzone = 0.1f;
//const float triggerDeadzone = 0.1f;

AppState InitSimulation()
{
    AppState state = {0};
    state.renderSettings.camera.position.z = -10.0f;
    state.renderSettings.camera.rotation = Quat::identity;
    return state;
}

bool IsGamepadStateNull(OS_GamepadState* gamepads, int idx)
{
    bool null = true;
    OS_GamepadState& gamepad = gamepads[idx];
    null &= gamepad.buttons == 0;
    null &= abs(gamepad.leftTrigger)  < triggerDeadzone;
    null &= abs(gamepad.rightTrigger) < triggerDeadzone;
    null &= abs(gamepad.leftStickX)   < stickDeadzone;
    null &= abs(gamepad.leftStickY)   < stickDeadzone;
    null &= abs(gamepad.rightStickX)  < stickDeadzone;
    null &= abs(gamepad.rightStickY)  < stickDeadzone;
    return !gamepads[idx].active || null;
}

InputDominator GetDominatingGamepad(OS_InputState input, InputDominator prevDom)
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

void ApplyDeadzone(OS_InputState* input)
{
    for(int i = 0; i < MaxActiveControllers; ++i)
    {
        OS_GamepadState& gamepad = input->gamepads[i];
        if(abs(gamepad.leftTrigger)  < triggerDeadzone) gamepad.leftTrigger  = 0;
        if(abs(gamepad.rightTrigger) < triggerDeadzone) gamepad.rightTrigger = 0;
        if(abs(gamepad.leftStickX)   < stickDeadzone)   gamepad.leftStickX   = 0;
        if(abs(gamepad.leftStickY)   < stickDeadzone)   gamepad.leftStickY   = 0;
        if(abs(gamepad.rightStickX)  < stickDeadzone)   gamepad.rightStickX  = 0;
        if(abs(gamepad.rightStickY)  < stickDeadzone)   gamepad.rightStickY  = 0;
    }
}

InputDominator GetDominatingKeyboard(OS_InputState input, InputDominator prevDom)
{
    TODO;
    return prevDom;
}

void UpdateCamera(Transform* camera, float deltaTime)
{
    // Example API usage
    Input input = GetInput();
    
    // Camera rotation
    const float rotateXSpeed = Deg2Rad(120);
    const float rotateYSpeed = Deg2Rad(80);
    const float mouseSensitivity = Deg2Rad(0.05);  // Degrees per pixel
    static float angleX = 0.0f;
    static float angleY = 0.0f;
    
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    if(input.virtualKeys[Keycode_RMouse])
    {
        mouseX = input.mouseDelta.x * mouseSensitivity;
        mouseY = input.mouseDelta.y * mouseSensitivity;
    }
    
    angleX += rotateXSpeed * input.gamepad.rightStick.x * deltaTime + mouseX;
    angleY += rotateYSpeed * input.gamepad.rightStick.y * deltaTime + mouseY;
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
    
    float keyboardX = input.virtualKeys[Keycode_D] - input.virtualKeys[Keycode_A];
    float keyboardY = input.virtualKeys[Keycode_W] - input.virtualKeys[Keycode_S];
    Vec3 targetVel =
    {
        .x = (input.gamepad.leftStick.x + keyboardX) * moveSpeed,
        .y = 0.0f,
        .z = (input.gamepad.leftStick.y + keyboardY) * moveSpeed
    };
    
    if(dot(targetVel, targetVel) > moveSpeed * moveSpeed)
        targetVel = normalize(targetVel) * moveSpeed;
    
    targetVel = camera->rotation * targetVel;
    targetVel.y += (input.gamepad.rightTrigger - input.gamepad.leftTrigger) * moveSpeed;
    targetVel.y += (input.virtualKeys[Keycode_E] - input.virtualKeys[Keycode_Q]) * moveSpeed;
    
    curVel = MoveTowards(curVel, targetVel, moveAccel * deltaTime);
    camera->position += curVel * deltaTime;
}

void UpdateUI()
{
    Input input = GetInput();
    
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

void MainUpdate(AppState* state, float deltaTime, Arena* permArena, Arena* frameArena)
{
    PollAndProcessInput();
    
    UpdateUI();
    
    // Hide mouse cursor if right clicking on main window
    Input input = GetInput();
    if(input.virtualKeys[Keycode_RMouse])
    {
        if(!input.prev.virtualKeys[Keycode_RMouse])
        {
            state->lockMousePosX = input.mouseX;
            state->lockMousePosY = input.mouseY;
        }
        
        OS_ShowCursor(false);
        SetMousePos(state->lockMousePosX, state->lockMousePosY);
    }
    else if(input.prev.virtualKeys[Keycode_RMouse])
        OS_ShowCursor(true);
    
    UpdateCamera(&state->renderSettings.camera, deltaTime);
    
    // Render settings
    state->renderSettings.horizontalFOV = Deg2Rad(90);
    state->renderSettings.nearClipPlane = 0.5f;
    state->renderSettings.farClipPlane  = 1000.0f;
}
