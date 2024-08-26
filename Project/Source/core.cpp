
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
    
    Model* raptoidModel = GetModelByPath("Raptoid/Raptoid.model");
    Model* cubeModel = GetModelByPath("Common/Cube.model");
    
    raptoidModel->pipeline = GetPipelineByPath("CompiledShaders/model2proj.shader", "CompiledShaders/pbr.shader");
    cubeModel->pipeline = raptoidModel->pipeline;
    
    Model* sphereModel = GetModelByPath("Common/sphere.model");
    sphereModel->pipeline = raptoidModel->pipeline;
    
    Model* cylinderModel = GetModelByPath("Common/cylinder.model");
    cylinderModel->pipeline = raptoidModel->pipeline;
    
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
    camera->base->pos.z = -8.0f;
    camera->base->pos.y = 7.0f;
    camera->base->rot *= AngleAxis(Vec3::right, Deg2Rad(20.0f));
    camera->params.fov      = 90.0f;
    camera->params.nearClip = 0.1f;
    camera->params.farClip  = 1000.0f;
    
    res.mainCamera = GetKey(camera->base);
    
    auto player = NewEntity<Player>();
    player->base->model = cylinderModel;
    player->base->scale = {0.5f, 1.0f, 0.5f};
    player->gravity = 20.0f;
    player->jumpVel = 10.0f;
    player->moveSpeed = 4.0f;
    player->groundAccel = 40.0f;
    
    auto pointLight = NewEntity<PointLight>();
    
    float pos = 0.0f;
    
    {
        Entity* e = NewEntity();
        e->model = cubeModel;
        e->pos.x = pos;
        pos += 3.0f;
    }
    
    {
        Entity* e = NewEntity();
        e->model = sphereModel;
        e->pos.x = pos;
        pos += 3.0f;
    }
    
    {
        Entity* e[1000];
        for(int i = 0; i < 7; ++i)
        {
            e[i] = NewEntity();
            e[i]->model = raptoidModel;
            e[i]->pos.x = pos;
            pos += 3.0f;
        }
        
        MountEntity(e[4], e[3]);
        MountEntity(e[5], e[4]);
        MountEntity(e[6], e[5]);
    }
    
    // Scene params
    res.sceneParams.skyboxTop    = "Skybox/top.jpg";
    res.sceneParams.skyboxBottom = "Skybox/bottom.jpg";
    res.sceneParams.skyboxLeft   = "Skybox/left.jpg";
    res.sceneParams.skyboxRight  = "Skybox/right.jpg";
    res.sceneParams.skyboxFront  = "Skybox/front.jpg";
    res.sceneParams.skyboxBack   = "Skybox/back.jpg";
    
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

void MainUpdate(Entities* entities, Editor* editor, float deltaTime, Arena* permArena, Arena* frameArena)
{
    bool inEditor = false;
#ifdef Development
    inEditor = editor->inEditor;
#endif
    
    OS_DearImguiBeginFrame();
    
    PollAndProcessInput(inEditor);
    
    int width, height;
    OS_GetClientAreaSize(&width, &height);
    float aspectRatio = (float)width / (float)height;
    
    // Per frame computations
    {
        entities->liveChildrenPerEntity = ComputeAllLiveChildrenForEachEntity(frameArena);
    }
    defer { entities->liveChildrenPerEntity = {0}; };
    
    // Renderer beginning of frame stuff
    {
        R_SetViewport(width, height);
        R_ClearFrame({0.12f, 0.3f, 0.25f, 1.0f});
        
        Camera* cam = GetMainCamera();
        Vec3 camPos = {0};
        Quat camRot = Quat::identity;
        CameraParams camParams = {.nearClip=0.1f, .farClip=1000.0f};
        
        if(inEditor)
        {
            camPos    = editor->camPos;
            camRot    = editor->camRot;
            camParams = editor->camParams;
        }
        else if(cam)
        {
            camPos    = cam->base->pos;
            camRot    = cam->base->rot;
            camParams = cam->params;
        }
        else
        {
            Log("There is currently no active camera!");
        }
        
        Mat4 view2Proj = View2ProjMatrix(camParams.nearClip, camParams.farClip, camParams.fov, aspectRatio);
        R_PerFrameData perFrame =
        {
            World2ViewMatrix(camPos, camRot),
            R_ConvertView2ProjMatrix(view2Proj),
            camPos
        };
        R_SetPerFrameData(perFrame);
    }
    
    // Update
    if(inEditor)
    {
        UpdateEditor(editor, deltaTime);
    }
    else
    {
        UpdateEntities(entities, deltaTime);
    }
    
    // End of frame activities
    {
        CommitDestroy(entities);
    }
    
    // Render skybox
    {
        R_Pipeline pipeline = GetPipelineByPath("CompiledShaders/skybox_vertex.shader", "CompiledShaders/skybox_pixel.shader");
        R_SetPipeline(pipeline);
        R_Cubemap cubemap = GetCubemapByPath("Skybox/top.png", "Skybox/bottom.png",
                                             "Skybox/left.png", "Skybox/right.png",
                                             "Skybox/front.png", "Skybox/back.png");
        R_SetCubemap(cubemap, 0);
        
        Model* cube = GetModelByPath("Common/cube.model");
        R_CullFace(false);
        R_DepthTest(false);
        R_DrawModel(cube);
        R_DepthTest(true);
        R_CullFace(true);
    }
    
    // Render entities in the scene
    {
        for_live_entities(ent)
        {
            if(ent->model)
            {
                R_PerObjData perObj = { ComputeWorldTransform(ent) };
                R_SetPerObjData(perObj);
                
                R_SetPipeline(ent->model->pipeline);
                R_DrawModel(ent->model);
            }
        }
    }
    
    // Render and finalize editor
    if(inEditor)
    {
        RenderEditor(editor, deltaTime);
    }
    else
    {
        // Provide a way to go back to edit mode
        // TODO: In a more complete engine this would actually
        // reload the scene
        Input input = GetInput();
        if(PressedKey(input, Keycode_Esc))
        {
            editor->inEditor = true;
        }
    }
    
    ImGui::Render();  // Render UI on top of scene
    R_RenderDearImgui();
}

void UpdateEntities(Entities* entities, float deltaTime)
{
    Player* player = nullptr;
    
    for_live_derived(p, Player)
    {
        UpdatePlayer(p, deltaTime);
    }
}

void RenderEntities(Entities* entities, Editor* editor, bool inEditor, float deltaTime)
{
    for_live_entities(ent)
    {
        if(ent->model)
            R_DrawModel(ent->model);
    }
}

void RenderScene()
{
    // Render skybox
    {
        R_Pipeline pipeline = GetPipelineByPath("CompiledShaders/skybox_vertex.shader", "CompiledShaders/skybox_pixel.shader");
        R_SetPipeline(pipeline);
        R_Cubemap cubemap = GetCubemapByPath("Skybox/colorful/top.jpg", "Skybox/colorful/bottom.jpg",
                                             "Skybox/colorful/left.jpg", "Skybox/colorful/right.jpg",
                                             "Skybox/colorful/front.jpg", "Skybox/colorful/back.jpg");
        R_SetCubemap(cubemap, 0);
        
        Model* cube = GetModelByPath("Common/cube.model");
        R_CullFace(false);
        R_DepthTest(false);
        R_DrawModel(cube);
        R_DepthTest(true);
        R_CullFace(true);
    }
    
    // Render entities in the scene
    {
        for_live_entities(ent)
        {
            if(ent->model)
            {
                R_PerObjData perObj = { ComputeWorldTransform(ent) };
                R_SetPerObjData(perObj);
                
                R_SetPipeline(ent->model->pipeline);
                R_DrawModel(ent->model);
            }
        }
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

bool operator ==(EntityKey k1, EntityKey k2)
{
    return k1.id == k2.id && k1.gen == k2.gen;
}

bool operator !=(EntityKey k1, EntityKey k2)
{
    return k1.id != k2.id || k1.gen != k2.gen;
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

Entity* GetEntity(u32 id)
{
    return &entities.bases[id];
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

template<typename t>
t* GetDerived(Entity* entity)
{
    static_assert(!std::is_same_v<t, Entity>, "Use GetEntity instead.");
    
    if(!entity) return nullptr;
    
    if(entity->derivedKind != GetEntityKindFromType<t>()) return nullptr;
    
    Array<t>* derivedArray = GetArrayFromType<t>();
    return derivedArray->ptr + entity->derivedId;
}

void* GetDerivedAddr(Entity* entity)
{
    switch(entity->derivedKind)
    {
        case Entity_None: return nullptr;
        case Entity_Count: return nullptr;
        case Entity_Player: return (void*)&entities.players[entity->derivedId];
        case Entity_Camera: return (void*)&entities.cameras[entity->derivedId];
        case Entity_PointLight: return (void*)&entities.pointLights[entity->derivedId];
    }
    
    return nullptr;
}

EntityKey GetKey(Entity* entity)
{
    if(!entity) return NullKey();
    
    u32 id = (u32)(entity - entities.bases.ptr);
    u16 gen = entity->gen;
    return {id, gen};
}

u32 GetId(Entity* entity)
{
    if(!entity) return (u32)-1;
    
    u32 id = (u32)(entity - entities.bases.ptr);
    return id;
}

Entity* GetMount(Entity* entity)
{
    if(!entity) return nullptr;
    return GetEntity(entity->mount);
}

void MountEntity(Entity* entity, Entity* mountTo)
{
    if(!GetMount(entity) && !mountTo) return;
    
    // When mounting an entity, the transform
    // needs to be adjusted as to not have
    // any noticeable changes.
    
    // First, get the current world transform of both entities
    Mat4 worldEntity = ComputeWorldTransform(entity);
    if(!mountTo)
    {
        PosRotScaleFromMat4(worldEntity, &entity->pos, &entity->rot, &entity->scale);
        entity->mount = NullKey();
        return;
    }
    
    Mat4 worldMountTo = ComputeWorldTransform(mountTo);
    
    // Then turn it into a relative transform
    // with respect to the mounted entity
    worldEntity = inverse(worldMountTo) * worldEntity;
    PosRotScaleFromMat4(worldEntity, &entity->pos, &entity->rot, &entity->scale);
    
    entity->mount = GetKey(mountTo);
}

Mat4 ComputeWorldTransform(Entity* entity)
{
    if(!entity) return Mat4::identity;
    
    Mat4 transform = Mat4FromPosRotScale(entity->pos, entity->rot, entity->scale);
    transform = ComputeWorldTransform(GetMount(entity)) * transform;
    return transform;
}

Mat4 ConvertToLocalTransform(Entity* entity, Mat4 world)
{
    if(!entity) return world;
    
    Mat4 mountTransform = ComputeWorldTransform(GetMount(entity));
    return inverse(mountTransform) * world;
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

int GetNumEntities()
{
    return entities.bases.len;
}

Slice<Slice<Entity*>> GetLiveChildrenPerEntity()
{
    return entities.liveChildrenPerEntity;
}

void DestroyEntity(Entity* entity)
{
    if(!entity) return;
    entity->flags |= EntityFlags_Destroyed;
}

void CommitDestroy(Entities* entities)
{
    for(u32 i = 0; i < (u32)entities->bases.len; ++i)
    {
        Entity* ent = &entities->bases[i];
        u32 entId = i;
        
        bool toBeDestroyed = ent->flags & EntityFlags_Destroyed;
        if(!toBeDestroyed) // Check mounts as well
        {
            Entity* mount = ent;
            while(mount)
            {
                if(mount->flags & EntityFlags_Destroyed)
                {
                    toBeDestroyed = true;
                    break;
                }
                
                mount = GetMount(mount);
            }
        }
        
        if(toBeDestroyed)
        {
            // Destroy this entity completely (add it to the freeBases array)
            Append(&entities->freeBases, entId);
            ent->flags |= EntityFlags_Destroyed;
            
            // This nullifies all references to this entity
            ++ent->gen;
            
            // TODO: @leak Remove entity from derived type array as well
        }
    }
    
}

// Entity iteration
Entity* FirstLive()
{
    u32 id = 0;
    while(id < (u32)entities.bases.len && (entities.bases[id].flags & EntityFlags_Destroyed))
        ++id;
    
    if(id >= (u32)entities.bases.len) return nullptr;
    
    return &entities.bases[id];
}

Entity* NextLive(Entity* current)
{
    if(!current) return nullptr;
    
    u32 id = GetKey(current).id + 1;
    while(id < (u32)entities.bases.len && (entities.bases[id].flags & EntityFlags_Destroyed))
        ++id;
    
    if(id >= (u32)entities.bases.len) return nullptr;
    
    return &entities.bases[id];
}

template<typename t>
t* FirstDerivedLive()
{
    u32 id = 0;
    Array<t>* array = GetArrayFromType<t>();
    while(id < (u32)array->len && ((*array)[id].base->flags & EntityFlags_Destroyed))
        ++id;
    
    if(id >= (u32)array->len) return nullptr;
    
    return array->ptr + id;
}

template<typename t>
t* NextDerivedLive(t* current)
{
    u32 id = GetKey(current->base).id + 1;
    Array<t>* array = GetArrayFromType<t>();
    while(id < (u32)array->len && ((*array)[id].base->flags & EntityFlags_Destroyed))
        ++id;
    
    if(id >= (u32)array->len) return nullptr;
    
    return array->ptr + id;
}

Camera* GetMainCamera()
{
    return GetDerived<Camera>(entities.mainCamera);
}

Slice<Slice<Entity*>> ComputeAllLiveChildrenForEachEntity(Arena* dst)
{
    auto resPtr = ArenaZAllocArray(Slice<Entity*>, entities.bases.len, dst);
    Slice<Slice<Entity*>> res = {.ptr = resPtr, .len = entities.bases.len };
    
    ScratchArena scratch;
    auto ptr = ArenaZAllocArray(Array<Entity*>, entities.bases.len, dst);
    Slice<Array<Entity*>> childrenPerEntity = {.ptr = ptr, .len = entities.bases.len };
    defer
    {
        for(int i = 0; i < childrenPerEntity.len; ++i)
            Free(&childrenPerEntity[i]);
    };
    
    for_live_entities(ent)
    {
        Entity* mount = GetMount(ent);
        if(mount && !(mount->flags & EntityFlags_Destroyed))
        {
            u32 id = GetId(mount);
            Append(&childrenPerEntity[id], ent);
        }
    }
    
    for(int i = 0; i < childrenPerEntity.len; ++i)
        res[i] = CopyToArena(&childrenPerEntity[i], dst);
    return res;
}

bool IsChild(Entity* suspectedChild, Entity* entity, Slice<Slice<Entity*>> childrenPerEntity)
{
    Slice<Entity*> children = childrenPerEntity[GetId(entity)];
    for(int i = 0; i < children.len; ++i)
    {
        if(suspectedChild == children[i] || IsChild(suspectedChild, children[i], childrenPerEntity))
            return true;
    }
    
    return false;
}

void UpdatePlayer(Player* player, float deltaTime)
{
    Input input = GetInput();
    
    // Vertical movement
    {
        player->speed.y -= player->gravity * deltaTime;
        
        if(player->grounded)
        {
            player->speed.y = 0.0f;
        }
        
        bool pressedJump = PressedKey(input, Keycode_Space) || PressedGamepadButton(input, Gamepad_A);
        if(player->grounded && pressedJump)
        {
            player->speed.y = player->jumpVel;
            player->grounded = false;
        }
    }
    
    // Horizontal movement
    {
        Vec3 horizontalSpeed = player->speed;
        horizontalSpeed.y = 0.0f;
        
        float keyboardX = (float)(input.virtualKeys[Keycode_D] - input.virtualKeys[Keycode_A]);
        float keyboardZ = (float)(input.virtualKeys[Keycode_W] - input.virtualKeys[Keycode_S]);
        
        Vec3 targetVel =
        {
            .x = (input.gamepad.leftStick.x + keyboardX) * player->moveSpeed,
            .y = 0.0f,
            .z = (input.gamepad.leftStick.y + keyboardZ) * player->moveSpeed
        };
        
        if(dot(targetVel, targetVel) > player->moveSpeed * player->moveSpeed)
            targetVel = normalize(targetVel) * player->moveSpeed;
        
        horizontalSpeed = ApproachLinear(horizontalSpeed, targetVel, player->groundAccel * deltaTime);
        player->speed = horizontalSpeed + Vec3::up * player->speed.y;
    }
    
    player->base->pos += player->speed * deltaTime;
    
    // "Simulating" collision and ground detection
    if(player->base->pos.y <= 0.0f)
    {
        player->grounded = true;
        player->base->pos.y = 0.0f;
    }
}