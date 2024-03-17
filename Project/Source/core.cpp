
#include "os/os_generic.h"
#include "core.h"

AppState InitSimulation()
{
    AppState state = {0};
    state.renderSettings.camera.position.z = -10.0f;
    state.renderSettings.camera.rotation = Quat::identity;
    
    // Let's load the scene here
    Model* gunModel = LoadModelByName("gun_model");  // This also has "gun_model"/idx in there
    Model* raptoidModel = LoadModelByName("raptoid_model");
    
    // Let's organize the code with arenas later, right now i would just like to get
    // the general idea.
    auto entities = (Entity*)malloc(1000*sizeof(Entity));
    memset(entities, 0, sizeof(Entity)*1000);
    state.entities.len = 2;
    state.entities.ptr = entities;
    
    entities[0].id = 0;
    entities[1].id = 1;
    
    entities[0].rot = Quat::identity;
    entities[1].rot = Quat::identity;
    
    entities[0].model = gunModel;
    entities[1].model = raptoidModel;
    
    return state;
}

void UpdateEntities(AppState* state, float deltaTime)
{
    auto& entities = state->entities;
    
    // Update entity 0
    {
        auto& entity = entities[0];
        entity.pos.x += sin(deltaTime);
    }
    
    // Update entity 1
    {
        auto& entity = entities[1];
        entity.pos.y += sin(deltaTime);
    }
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
    
    float keyboardX = 0.0f;
    float keyboardY = 0.0f;
    if(input.virtualKeys[Keycode_RMouse])
    {
        keyboardX = input.virtualKeys[Keycode_D] - input.virtualKeys[Keycode_A];
        keyboardY = input.virtualKeys[Keycode_W] - input.virtualKeys[Keycode_S];
    }
    
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
    UpdateEntities(state, deltaTime);
    
    // Render settings
    state->renderSettings.horizontalFOV = Deg2Rad(90);
    state->renderSettings.nearClipPlane = 0.5f;
    state->renderSettings.farClipPlane  = 1000.0f;
}

void MainRender(AppState* appState, RenderSettings settings)
{
    glClearColor(0.6f, 0.4f, 0.6f, 1.0f);
    
    auto& entities = appState->entities;
    for(int i = 0; i < entities.len; ++i)
    {
        if(entities[i].model)
            RenderModel(entities[i].model);
    }
}
