
#include "os/os_generic.h"
#include "core.h"
#include "asset_system.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"

static Entities entities;
Entities* InitEntities()
{
    entities = {0};
    auto& res = entities;
    
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
    
    static Arena baseArena   = ArenaVirtualMemInit(GB(4), MB(2));
    static Arena cameraArena = ArenaVirtualMemInit(MB(64), MB(2));
    static Arena playerArena = ArenaVirtualMemInit(MB(64), MB(2));
    static Arena pointLightArena = ArenaVirtualMemInit(GB(1), MB(2));
    UseArena(&res.bases, &baseArena);
    UseArena(&res.cameras, &cameraArena);
    UseArena(&res.players, &playerArena);
    UseArena(&res.pointLights, &pointLightArena);
    static_assert(4 == Entity_Count, "Every array for each type should be based on an arena for pointer stability");
    
    auto raptoid = NewEntity();
    raptoid->model = raptoidModel;
    
    auto camera = NewEntity<Camera>();
    camera->horizontalFOV = 90.0f;
    camera->nearClip = 0.1f;
    camera->farClip = 1000.0f;
    
    res.mainCamera = GetKey(camera->base);
    
    auto player = NewEntity<Player>();
    player->base->model = raptoidModel;
    auto pointLight = NewEntity<PointLight>();
    return &res;
}

void FreeEntities(Entities* entities)
{
    Free(&entities->bases);
    Free(&entities->cameras);
    Free(&entities->players);
    Free(&entities->pointLights);
    static_assert(4 == Entity_Count, "All entity type arrays should be freed");
}

void MainUpdate(Entities* entities, float deltaTime, Arena* permArena, Arena* frameArena)
{
    OS_DearImguiBeginFrame();
    
    PollAndProcessInput();
    
    UpdateUI();
    
    // Test entity destruction
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
    ImGui::Render();  // Render UI on top of scene
    R_RenderDearImgui();
    
    CommitDestroy(entities);
}

void UpdateEntities(Entities* entities, float deltaTime)
{
    Player* player = nullptr;
    
    for_live_derived(p, Player)
    {
        UpdatePlayer(p, deltaTime);
    }
    
    Camera* mainCamera = GetDerived<Camera>(entities->mainCamera);
    if(mainCamera)
        UpdateMainCamera(mainCamera, deltaTime);
}

void UpdateUI()
{
    static bool open = true;
    if(open)
        ImGui::ShowDemoWindow(&open);
}

void RenderEntities(Entities* entities, float deltaTime)
{
    Camera* camera = GetDerived<Camera>(entities->mainCamera);
    if(!camera) return;
    
    R_BeginPass(camera);
    
    for_live_entities(ent)
    {
        if(ent->model)
            R_DrawModel(ent->model, ComputeWorldTransform(ent));
    }
}

// Entity manipulation

template<typename t>
EntityKind GetEntityKindFromType()
{
    if constexpr (std::is_same_v<t, Camera>)
        return Entity_Camera;
    else if constexpr (std::is_same_v<t, Player>)
        return Entity_Player;
    else if constexpr (std::is_same_v<t, PointLight>)
        return Entity_PointLight;
    else
        static_assert(false, "This type is not derived from entity");
    
    static_assert(4 == Entity_Count, "Every derived type must have a corresponding if");
}

template<typename t>
Array<t>* GetArrayFromType()
{
    if constexpr (std::is_same_v<t, Entity>)
        return &entities.bases;
    else if constexpr (std::is_same_v<t, Camera>)
        return &entities.cameras;
    else if constexpr (std::is_same_v<t, Player>)
        return &entities.players;
    else if constexpr (std::is_same_v<t, PointLight>)
        return &entities.pointLights;
    else
        static_assert(false, "This type is not derived from entity");
    
    static_assert(4 == Entity_Count, "Every derived type must have a corresponding if");
}

EntityKey NullKey()
{
    return {.id=(u32)-1, .gen=0};
}

bool IsKeyNull(EntityKey key)
{
    EntityKey null = NullKey();
    return key.id == null.id && key.gen == null.gen; 
}

Entity* GetEntity(EntityKey key)
{
    if(IsKeyNull(key)) return nullptr;
    
    Entity* res = &entities.bases[key.id];
    if(res->gen != key.gen) return nullptr;
    
    return res;
}

template<typename t>
t* GetDerived(EntityKey key)
{
    static_assert(!std::is_same_v<t, Entity>, "Use GetEntity instead.");
    
    if(IsKeyNull(key)) return nullptr;
    
    Entity* res = &entities.bases[key.id];
    if(res->gen != key.gen) return nullptr;
    
    if(res->derivedKind != GetEntityKindFromType<t>()) return nullptr;
    
    Array<t>* derivedArray = GetArrayFromType<t>();
    return derivedArray->ptr + res->derivedId;
}

EntityKey GetKey(Entity* entity)
{
    if(!entity) return NullKey();
    
    u32 id = entity - entities.bases.ptr;
    u16 gen = entity->gen;
    return {id, gen};
}

Entity* GetMount(Entity* entity)
{
    return GetEntity(entity->mount);
}

void MountEntity(Entity* entity, Entity* mountTo)
{
    entity->mount = GetKey(mountTo);
}

Mat4 ComputeWorldTransform(Entity* entity)
{
    if(!entity) return Mat4::identity;
    
    Mat4 transform = Mat4FromPosRotScale(entity->pos, entity->rot, entity->scale);
    transform = ComputeWorldTransform(GetMount(entity)) * transform;
    return transform;
}

Entity* NewEntity()
{
    u32 freeSlot = 0;
    if(entities.freeBases.len > 0)
    {
        freeSlot = entities.freeBases[entities.freeBases.len - 1];
        --entities.freeBases.len;
        
        // Wipe out everything except for the generation
        u32 gen = entities.bases[freeSlot].gen;
        entities.bases[freeSlot] = {0};
        entities.bases[freeSlot].gen = gen;
    }
    else
    {
        Append(&entities.bases, {0});
        freeSlot = entities.bases.len - 1;
    }
    
    Entity* entity = &entities.bases[freeSlot];
    entity->pos = {0};
    entity->rot = Quat::identity;
    entity->scale = {.x=1.0f, .y=1.0f, .z=1.0f};
    entity->mount = NullKey();
    return entity;
}

// Default constructor for a given derived type
template<typename t>
t* NewEntity()
{
    if constexpr (std::is_same_v<t, Entity>)
    {
        return NewEntity();
    }
    else
    {
        auto base = NewEntity();
        
        Array<t>* array = GetArrayFromType<t>();
        Append(array, {0});
        
        t* derived = array->ptr + array->len - 1;
        derived->base = base;
        base->derivedKind = GetEntityKindFromType<t>();
        base->derivedId = array->len - 1;
        return derived;
    }
}

void DestroyEntity(Entity* entity)
{
    if(!entity) return;
    
    entity->flags |= EntityFlags_Destroyed;
    
    // We increase the generation so that all
    // references to this are now invalid
    ++entity->gen;
    
    Append(&entities.freeBases, GetKey(entity).id);
}

void CommitDestroy(Entities* entities)
{
    // TODO: stuff missing here
}

// Entity iteration
Entity* FirstLive()
{
    u32 id = 0;
    while(id < entities.bases.len && (entities.bases[id].flags & EntityFlags_Destroyed))
        ++id;
    
    if(id >= entities.bases.len) return nullptr;
    
    return &entities.bases[id];
}

Entity* NextLive(Entity* current)
{
    if(!current) return nullptr;
    
    u32 id = GetKey(current).id + 1;
    while(id < entities.bases.len && (entities.bases[id].flags & EntityFlags_Destroyed))
        ++id;
    
    if(id >= entities.bases.len) return nullptr;
    
    return &entities.bases[id];
}

template<typename t>
t* FirstDerivedLive()
{
    u32 id = 0;
    Array<t>* array = GetArrayFromType<t>();
    while(id < array->len && ((*array)[id].base->flags & EntityFlags_Destroyed))
        ++id;
    
    if(id >= array->len) return nullptr;
    
    return array->ptr + id;
}

template<typename t>
t* NextDerivedLive(t* current)
{
    u32 id = GetKey(current->base).id + 1;
    Array<t>* array = GetArrayFromType<t>();
    while(id < array->len && ((*array)[id].base->flags & EntityFlags_Destroyed))
        ++id;
    
    if(id >= array->len) return nullptr;
    
    return array->ptr + id;
}

void UpdateMainCamera(Camera* camera, float deltaTime)
{
#if 1
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
#endif
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
