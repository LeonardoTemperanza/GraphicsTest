
#include "os/os_generic.h"
#include "core.h"
#include "asset_system.h"

Entities InitEntities()
{
    Entities res = {0};
    
    // Let's load the scene here
    //Model* gunModel = LoadModel("Gun/Gun.model");
    Model* raptoidModel = LoadModel("Raptoid/Raptoid.model");
    
    raptoidModel->vertex  = LoadShader("Shaders/simple_vertex.shader");
    raptoidModel->pixel   = LoadShader("Shaders/simple_pixel.shader");
    R_Shader shaders[] = {raptoidModel->vertex->handle, raptoidModel->pixel->handle};
    raptoidModel->program = R_LinkShaders({.ptr=shaders, .len=ArrayCount(shaders)});
    
    //SetMaterial(raptoidModel, raptoidMaterial, 0);
    //SetMaterial(raptoidModel, raptoidMaterial, 1);
    
    //Model* sphereModel = LoadModelAsset("Common/sphere.model");
    Model* sphereModel = raptoidModel;
    sphereModel->vertex = raptoidModel->vertex;
    sphereModel->pixel  = raptoidModel->pixel;
    sphereModel->program = raptoidModel->program;
    
    // In this entity system index 0 is considered null,
    // so we occupy the first slot of each type.
    // We use arenas to maintain stable pointers
    static Arena baseArena   = ArenaVirtualMemInit(GB(4), MB(2));
    static Arena cameraArena = ArenaVirtualMemInit(MB(64), MB(2));
    static Arena playerArena = ArenaVirtualMemInit(MB(64), MB(2));
    UseArena(&res.bases.all, &baseArena);
    UseArena(&res.cameras.all, &cameraArena);
    UseArena(&res.players.all, &playerArena);
    Append(&res.bases.all,   {0});
    Append(&res.cameras.all, {0});
    Append(&res.players.all, {0});
    static_assert(3 == Entity_Count, "Every array for each type should have a zero-initialized element on index 0");
    
    u32 e1 = NewEntity(&res);
    Entity& ent1 = res.bases.all[e1];
    ent1.pos = {.x=1.0f, .y=0.0f, .z=3.0f};
    ent1.rot = Quat::identity;
    
    u16 player = NewDerived(&res, &res.players);
    Player& p = res.players.all[player];
    p.base->model = raptoidModel;
    
    // Set main camera
    u16 cam = NewDerived(&res, &res.cameras);
    Camera& c = res.cameras.all[cam];
    c.base->pos = {.x=0.0f, .y=0.0f, .z=-5.0f};
    c.base->rot = Quat::identity;
    c.horizontalFOV = 90.0f;
    c.nearClip = 0.1f;
    c.farClip = 1000.0f;
    
    res.mainCamera = {.id=cam, .gen=c.base->gen};
    
    return res;
}

template<typename t>
void Free(DerivedArray<t>* array)
{
    Free(&array->all);
    Free(&array->free);
}

void Free(BaseArray* array)
{
    Free(&array->all);
    Free(&array->free);
}

void FreeEntities(Entities* entities)
{
    Free(&entities->bases);
    Free(&entities->cameras);
    Free(&entities->players);
    static_assert(3 == Entity_Count, "All entity type arrays should be freed");
}

void MainUpdate(Entities* entities, float deltaTime, Arena* permArena, Arena* frameArena)
{
    PollAndProcessInput();
    
    UpdateUI();
    
    static float time = 0.0f;
    static bool destroyed = false;
    time += deltaTime;
    if(time >= 1.0f && !destroyed)
    {
        //DeferDestroyEntity(entities, 2);
        destroyed = true;
    }
    
    // Hide mouse cursor if right clicking on main window
    Input input = GetInput();
    if(input.virtualKeys[Keycode_RMouse])
    {
        OS_ShowCursor(false);
        OS_FixCursor(true);
    }
    else
    {
        OS_ShowCursor(true);
        OS_FixCursor(false);
    }
    
    UpdateEntities(entities, deltaTime);
    RenderEntities(entities, deltaTime);
}

void UpdateEntities(Entities* entities, float deltaTime)
{
    /*
    auto& players = entities->players.all;
    for(int i = 1; i < players.len; ++i)
    {
        if(!(players[i].base->flags & EntityFlags_Destroyed))
        {
            UpdatePlayer(&players[i], deltaTime);
        }
    }
    */
    
    UpdatePlayer(&entities->players.all[1], deltaTime);
    UpdateMainCamera(&entities->cameras.all[1], deltaTime);
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

void RenderEntities(Entities* entities, float deltaTime)
{
    u32 cam = LookupEntity(entities, entities->mainCamera);
    if(cam == 0) return;
    
    R_BeginPass(&entities->cameras.all[1]);
    
    auto& ents = entities->bases.all;
    for(int i = 1; i < ents.len; ++i)
    {
        if(ents[i].model && !(ents[i].flags & EntityFlags_Destroyed))
            R_DrawModel(ents[i].model, ComputeWorldTransform(entities, i));
    }
}

u32 NewEntity(Entities* entities)
{
    auto bases = &entities->bases;
    u16 freeSlot = 0;
    if(bases->free.len > 0)
    {
        freeSlot = bases->free[bases->free.len - 1];
        --bases->free.len;
        
        // Wipe out everything except for the generation
        u16 gen = bases->all[freeSlot].gen;
        bases->all[freeSlot] = {0};
        bases->all[freeSlot].gen = gen;
    }
    else
    {
        Append(&bases->all, {0});
        freeSlot = bases->all.len - 1;
    }
    
    auto& newEntity = bases->all[freeSlot];
    newEntity.rot   = Quat::identity;
    newEntity.scale = {.x=1.0f, .y=1.0f, .z=1.0f};
    return freeSlot;
}

template<typename t>
u16 NewDerived(Entities* entities, DerivedArray<t>* derived)
{
    u32 base = NewEntity(entities);
    Entity* entity = &entities->bases.all[base];
    
    u16 freeSlot = 0;
    if(derived->free.len > 0)
    {
        freeSlot = derived->free[derived->free.len - 1];
        --derived->free.len;
    }
    else
    {
        Append(&derived->all, {0});
        freeSlot = derived->all.len - 1;
    }
    
    derived->all[freeSlot].base = entity;
    return freeSlot;
}

void DestroyEntity(Entities* entities, u32 id)
{
    auto& ents = entities->bases.all;
    ents[id].flags |= EntityFlags_Destroyed;
    
    // We increase the generation so that all
    // references to this are now invalid
    ++ents[id].gen;
    
    // Notify the derived array as well of the destruction
    switch(ents[id].derivedKind)
    {
        case Entity_None: break;
        case Entity_Camera: Append(&entities->cameras.free, ents[id].derivedId); break;
        case Entity_Player: Append(&entities->players.free, ents[id].derivedId); break;
        case Entity_Count: break;
    }
    static_assert(3 == Entity_Count, "Update the switch statement to include all derived types");
    
    // Append the index to the curresponding "free slots" array
    Append(&entities->bases.free, id);
}

u32 LookupEntity(Entities* entities, EntityKey key)
{
    if(key.id == NullId) return NullId;
    
    Entity& entity = entities->bases.all[key.id];
    if(entity.gen != key.gen)
        return 0;
    
    return key.id;
}

u32 LookupEntity(Entities* entities, EntityKey key, EntityKind kind)
{
    if(key.id == NullId) return NullId;
    
    Entity& entity = entities->bases.all[key.id];
    if(entity.gen != key.gen || entity.derivedKind != kind)
        return 0;
    
    return key.id;
}

void MountEntity(Entities* entities, u32 mounted, u32 mountTo)
{
    assert(mounted > entities->bases.all.len && mountTo > entities->bases.all.len);
    
    Entity& mountedEnt = entities->bases.all[mounted];
    Entity& mountToEnt = entities->bases.all[mountTo];
    mountedEnt.mount.id  = mountTo;
    mountedEnt.mount.gen = mountToEnt.gen;
}

Mat4 ComputeWorldTransform(Entities* entities, u32 id)
{
    if(id == 0) return Mat4::identity;
    
    auto& ents = entities->bases.all;
    Mat4 transform = Mat4FromPosRotScale(ents[id].pos, ents[id].rot, ents[id].scale);
    u32 mount = LookupEntity(entities, ents[id].mount);
    transform = ComputeWorldTransform(entities, mount) * transform;
    return transform;
}

void UpdateMainCamera(Camera* camera, float deltaTime)
{
    return;
    
    Input input = GetInput();
    
    // Camera rotation
    const float rotateXSpeed = Deg2Rad(120);
    const float rotateYSpeed = Deg2Rad(80);
    const float mouseSensitivity = Deg2Rad(0.08);  // Degrees per pixel
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
    
    angleY = clamp(angleY, Deg2Rad(-90), Deg2Rad(90));
    
    Quat yRot = AngleAxis(Vec3::left, angleY);
    Quat xRot = AngleAxis(Vec3::up, angleX);
    camera->base->rot = xRot * yRot;
    
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
    
    targetVel = camera->base->rot * targetVel;
    targetVel.y += (input.gamepad.rightTrigger - input.gamepad.leftTrigger) * moveSpeed;
    targetVel.y += (input.virtualKeys[Keycode_E] - input.virtualKeys[Keycode_Q]) * moveSpeed;
    
    curVel = ApproachLinear(curVel, targetVel, moveAccel * deltaTime);
    camera->base->pos += curVel * deltaTime;
}

void UpdatePlayer(Player* player, float deltaTime)
{
    const float moveSpeed = 4.0f;
    const float moveAccel = 30.0f;
    static Vec3 curVel = {0};
    
    Input input = GetInput();
    
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
    
    targetVel.y += (input.gamepad.rightTrigger - input.gamepad.leftTrigger) * moveSpeed;
    targetVel.y += (input.virtualKeys[Keycode_E] - input.virtualKeys[Keycode_Q]) * moveSpeed;
    
    curVel = ApproachLinear(curVel, targetVel, moveAccel * deltaTime);
    player->base->pos += curVel * deltaTime;
}
