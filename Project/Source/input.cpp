
#include "base.h"
#include "input.h"

// Will be adjustable settings in the future
static const float stickDeadzone = 0.1f;
static const float triggerDeadzone = 0.1f;
static InputCtx inputCtx;

void PollAndProcessInput()
{
    auto& curInput = inputCtx.curInput;
    
    // Current frame's input becomes last frame's
    curInput.prev.gamepad    = curInput.gamepad;
    memcpy(curInput.prev.virtualKeys, curInput.virtualKeys, sizeof(curInput.virtualKeys));
    curInput.prev.mouseX     = curInput.mouseX;
    curInput.prev.mouseY     = curInput.mouseY;
    curInput.prev.mouseDelta = curInput.prev.mouseDelta;
    
    OS_InputState input = OS_PollInput();
    
    // Compute deltas
    if(inputCtx.followingPolls)
    {
        curInput.mouseDelta.x =   input.mouse.xPos - curInput.mouseX;
        // Down is positive, but the convention for delta is, it's negative
        curInput.mouseDelta.y = -(input.mouse.yPos - curInput.mouseY);
    }
    else  // This is the first input poll
        curInput.mouseDelta = {.x=0.0f, .y=0.0f};
    
    // Set cursor position if needed
    if(inputCtx.setMousePos)
    {
        OS_SetCursorPos(inputCtx.deferredMousePosX, inputCtx.deferredMousePosY);
        curInput.mouseX = inputCtx.deferredMousePosX;
        curInput.mouseY = inputCtx.deferredMousePosY;
        inputCtx.setMousePos = false;
    }
    else
    {
        curInput.mouseX = input.mouse.xPos;
        curInput.mouseY = input.mouse.yPos;
    }
    
    // Update input state
    memcpy(curInput.virtualKeys, input.virtualKeys, sizeof(input.virtualKeys));
    
    // Process gamepad input
    for(int i = 0; i < MaxActiveControllers; ++i)
        ApplyDeadzones(&input.gamepads[i]);
    
    InputDominator prevDom = inputCtx.gamepadDom;
    inputCtx.gamepadDom = FindDominatingGamepad(input, prevDom);
    curInput.changedDom = prevDom.idx != inputCtx.gamepadDom.idx && prevDom.active && inputCtx.gamepadDom.active;
    
    auto domGP = input.gamepads[inputCtx.gamepadDom.idx];
    curInput.gamepad.buttons      = domGP.buttons;
    curInput.gamepad.leftTrigger  = domGP.leftTrigger;
    curInput.gamepad.rightTrigger = domGP.rightTrigger;
    curInput.gamepad.leftStick.x  = domGP.leftStickX;
    curInput.gamepad.leftStick.y  = domGP.leftStickY;
    curInput.gamepad.rightStick.x = domGP.rightStickX;
    curInput.gamepad.rightStick.y = domGP.rightStickY;
    
    // Done at least one poll
    inputCtx.followingPolls = true;
}

Input GetInput()
{
    return inputCtx.curInput;
}



void SetMousePos(s64 mouseX, s64 mouseY)
{
    inputCtx.deferredMousePosX = mouseX;
    inputCtx.deferredMousePosY = mouseY;
    inputCtx.setMousePos = true;
}

void ApplyDeadzones(OS_GamepadState* gamepad)
{
    if(abs(gamepad->leftTrigger)  < triggerDeadzone) gamepad->leftTrigger  = 0;
    if(abs(gamepad->rightTrigger) < triggerDeadzone) gamepad->rightTrigger = 0;
    
    if(sqr(gamepad->leftStickX) + sqr(gamepad->leftStickY) <= sqr(stickDeadzone))
    {
        gamepad->leftStickX = 0;
        gamepad->leftStickY = 0;
    }
    
    if(sqr(gamepad->rightStickX) + sqr(gamepad->rightStickY) <= sqr(stickDeadzone))
    {
        gamepad->rightStickX = 0;
        gamepad->rightStickY = 0;
    }
}

bool IsGamepadStateNull(OS_GamepadState gamepadState)
{
    bool null = true;
    auto gp = gamepadState;
    null &= gp.buttons == 0;
    null &= abs(gp.leftTrigger)  < triggerDeadzone;
    null &= abs(gp.rightTrigger) < triggerDeadzone;
    null &= abs(gp.leftStickX)   < stickDeadzone;
    null &= abs(gp.leftStickY)   < stickDeadzone;
    null &= abs(gp.rightStickX)  < stickDeadzone;
    null &= abs(gp.rightStickY)  < stickDeadzone;
    return !gp.active || null;
}

InputDominator FindDominatingGamepad(OS_InputState input, InputDominator prevDom)
{
    // By default return the previous one
    InputDominator res = prevDom;
    
    // Find out if its input is null this frame.
    // If it is, it could potentially be dominated
    // by some other gamepad.
    bool gamepadStateNull = IsGamepadStateNull(input.gamepads[prevDom.idx]);
    bool checkForDominator = !prevDom.active || (prevDom.active && gamepadStateNull);
    if(checkForDominator)
    {
        for(int i = 0; i < MaxActiveControllers; ++i)
        {
            if(prevDom.active && i == prevDom.idx) continue;
            
            if(!IsGamepadStateNull(input.gamepads[i]))
            {
                res.active = true;
                res.idx = i;
                break;
            }
        }
    }
    
    return res;
}
