
#include "os/os_generic.h"
#include "core.h"
#include "asset_system.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"

// TODO: This will not be necessary once scene serialization is ready
EntityManager InitEntityManager()
{
    EntityManager manager = {};
    EntityManager* man = &manager;
    
    const char* raptoidPath  = "Raptoid/Raptoid_2.mesh";
    const char* cubePath     = "Common/Cube.mesh";
    const char* spherePath   = "Common/Cube.mesh";
    const char* cylinderPath = "Common/cylinder.mesh";
    const char* pbr = "pbr.mat";
    
    static Arena baseArena   = ArenaVirtualMemInit(GB(4), MB(2));
    static Arena cameraArena = ArenaVirtualMemInit(MB(64), MB(2));
    static Arena playerArena = ArenaVirtualMemInit(MB(64), MB(2));
    static Arena pointLightArena = ArenaVirtualMemInit(GB(1), MB(2));
    UseArena(&man->bases, &baseArena);
    UseArena(&man->cameras, &cameraArena);
    UseArena(&man->players, &playerArena);
    UseArena(&man->pointLights, &pointLightArena);
    static_assert(4 == Entity_Count, "Every array for each type should be based on an arena for pointer stability");
    
    // When we'll actually load the scene we won't do a string lookup
    // for each entity because that's stupid... We'll have the handle already
    // saved, as well as the refcount for each asset already saved.
    
    auto raptoid = NewEntity(man);
    raptoid->mesh = AcquireMesh(raptoidPath);
    //raptoid->material = raptoidMat;
    
    auto quadEnt = NewEntity(man);
    quadEnt->mesh = AcquireMesh(cubePath);
    //quadEnt->material = raptoidMat;
    
    auto camera = NewEntity<Camera>(man);
    camera->base->flags |= EntityFlags_NoMesh;
    camera->base->pos.z = -8.0f;
    camera->base->pos.y = 7.0f;
    camera->base->rot *= AngleAxis(Vec3::right, Deg2Rad(20.0f));
    //camera->params.fov      = 90.0f;
    //camera->params.nearClip = 0.1f;
    //camera->params.farClip  = 1000.0f;
    
    man->mainCamera = GetKey(man, camera->base);
    
    auto player = NewEntity<Player>(man);
    player->base->mesh = AcquireMesh(cylinderPath);
    //player->base->material = raptoidMat;
    player->base->scale = {0.5f, 1.0f, 0.5f};
    player->gravity = 20.0f;
    player->jumpVel = 10.0f;
    player->moveSpeed = 4.0f;
    player->groundAccel = 40.0f;
    
    auto pointLight = NewEntity<PointLight>(man);
    pointLight->base->flags |= EntityFlags_NoMesh;
    
    float pos = 0.0f;
    
    {
        Entity* e   = NewEntity(man);
        e->mesh     = AcquireMesh(spherePath);
        //e->material = raptoidMat;
        e->pos.x = pos;
        e->pos.z = -3.0f;
        pos += 3.0f;
    }
    
    {
        Entity* e = NewEntity(man);
        e->mesh = AcquireMesh(spherePath);
        //e->material = raptoidMat;
        e->pos.x = pos;
        pos += 3.0f;
    }
    
    {
        Entity* e[1000];
        for(int i = 0; i < 7; ++i)
        {
            e[i] = NewEntity(man);
            e[i]->mesh = AcquireMesh(raptoidPath);
            //e[i]->material = raptoidMat;
            e[i]->pos.x = pos;
            pos += 3.0f;
        }
        
        MountEntity(man, e[4], e[3]);
        MountEntity(man, e[5], e[4]);
        MountEntity(man, e[6], e[5]);
    }
    
    return manager;
}

void FreeEntities(EntityManager* man)
{
    // TODO: This should be generated by the metaprogram
    Free(&man->bases);
    Free(&man->cameras);
    Free(&man->players);
    Free(&man->pointLights);
    static_assert(4 == Entity_Count, "All entity type arrays should be freed");
}

void MainUpdate(EntityManager* man, Editor* editor, float deltaTime, Arena* frameArena, CamParams* outCam)
{
    bool inEditor = false;
#ifdef Development
    inEditor = editor->inEditor;
#endif
    
    OS_DearImguiBeginFrame();
    R_ImGuiNewFrame();
    ImGui::NewFrame();
    
    PollAndProcessInput(inEditor);
    
#ifdef Development
    //HotReloadAssets(frameArena);
#endif
    
    int width, height;
    OS_GetClientAreaSize(&width, &height);
    float aspectRatio = (float)width / (float)height;
    
    // Per frame computations
    {
        man->liveChildrenPerEntity = ComputeAllLiveChildrenForEachEntity(man, frameArena);
    }
    
    // Update
    if(inEditor)
    {
        UpdateEditor(editor, deltaTime);
        outCam->pos = editor->camPos;
        outCam->rot = editor->camRot;
        outCam->fov = editor->camParams.fov;
        outCam->nearClip = editor->camParams.nearClip;
        outCam->farClip = editor->camParams.farClip;
    }
    else
    {
        UpdateEntities(man, deltaTime);
        TODO;
        outCam->pos = editor->camPos;
        outCam->rot = editor->camRot;
        outCam->fov = editor->camParams.fov;
        outCam->nearClip = editor->camParams.nearClip;
        outCam->farClip = editor->camParams.farClip;
    }
    
    // End of frame activities
    {
        CommitDestroy(man);
    }
}

#if 0
void MainRender(EntityManager* man, Editor* editor, float deltaTime, Arena* frameArena)
{
    int width, height;
    OS_GetClientAreaSize(&width, &height);
    float aspectRatio = (float)width / (float)height;
    
    //R_WaitLastFrameAndBeginCurrentFrame();
    
    if(width <= 0 || height <= 0)
    {
        // If these are not called anyway,
        // an assert fails.
        ImGui::Render();
        R_DearImguiRender();
        return;
    }
    
    bool inEditor = false;
#ifdef Development
    inEditor = editor->inEditor;
#endif
    
    // Beginning of frame prepping
    {
        R_SetViewport(width, height);
        R_ClearFrame({0.12f, 0.3f, 0.25f, 1.0f});
        
        Camera* cam = GetDerived<Camera>(man, man->mainCamera);
        Vec3 camPos = {0};
        Quat camRot = Quat::identity;
        R_CameraParams camParams = {};
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
            camPos    = Vec3::zero;
            camRot    = Quat::identity;
            camParams.fov      = 90.0f;
            camParams.nearClip = 0.1f;
            camParams.farClip  = 1000.0f;
            
            Log("There is currently no active camera!");
        }
        
        Mat4 view2Proj = View2ProjPerspectiveMatrix(camParams.nearClip, camParams.farClip, camParams.fov, aspectRatio);
        R_SetPerFrameData(World2ViewMatrix(camPos, camRot), R_ConvertClipSpace(view2Proj), camPos);
    }
    
#if 0
    // Render skybox
    {
        /*R_SetPipeline(pipeline);
        R_Pipeline pipeline = GetPipelineByPath("CompiledShaders/skybox_vertex.shader", "CompiledShaders/skybox_pixel.shader");
        R_Cubemap cubemap = GetCubemapByPath("Skybox/sky2.png");
        R_SetCubemap(cubemap, 0);
        */
        
        //R_Mesh cube = GetMeshByPath("Common/cube.mesh");
        R_CullFace(false);
        R_DepthTest(false);
        //R_DrawMesh(cube);
        R_DepthTest(true);
        R_CullFace(true);
    }
#endif
    
    // Render entities in the scene
    {
        R_SetSampler(R_SamplerDefault, ShaderKind_Pixel, CodeSampler0);
        
        ShaderHandle vertShader = GetShaderByPath("CompiledShaders/model2proj.shader", ShaderKind_Vertex);
        R_SetVertexShader(vertShader);
        
        for_live_entities(man, ent)
        {
            Mat4 model = ComputeWorldTransform(man, ent);
            Mat3 normal = ToMat3(transpose(ComputeTransformInverse(model)));
            R_SetPerObjData(model, normal);
            UseMaterial(ent->material);
            R_DrawMesh(ent->mesh);
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
    R_DearImguiRender();
}
#endif

void UpdateEntities(EntityManager* man, float deltaTime)
{
    Player* player = nullptr;
    
    for_live_derived(man, p, Player)
    {
        UpdatePlayer(p, deltaTime);
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
Array<t>* GetArrayFromType(EntityManager* man)
{
    if constexpr (std::is_same_v<t, Entity>)
        return &man->bases;
    else if constexpr (std::is_same_v<t, Camera>)
        return &man->cameras;
    else if constexpr (std::is_same_v<t, Player>)
        return &man->players;
    else if constexpr (std::is_same_v<t, PointLight>)
        return &man->pointLights;
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

Entity* GetEntity(EntityManager* man, EntityKey key)
{
    if(IsKeyNull(key)) return nullptr;
    
    Entity* res = &man->bases[key.id];
    if(res->gen != key.gen) return nullptr;
    
    return res;
}

Entity* GetEntity(EntityManager* man, u32 id)
{
    return &man->bases[id];
}

template<typename t>
t* GetDerived(EntityManager* man, EntityKey key)
{
    static_assert(!std::is_same_v<t, Entity>, "Use GetEntity instead.");
    
    if(IsKeyNull(key)) return nullptr;
    
    Entity* res = &man->bases[key.id];
    if(res->gen != key.gen) return nullptr;
    
    if(res->derivedKind != GetEntityKindFromType<t>()) return nullptr;
    
    Array<t>* derivedArray = GetArrayFromType<t>(man);
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

void* GetDerivedAddr(EntityManager* man, Entity* entity)
{
    switch(entity->derivedKind)
    {
        case Entity_None: return nullptr;
        case Entity_Count: return nullptr;
        case Entity_Player: return (void*)&man->players[entity->derivedId];
        case Entity_Camera: return (void*)&man->cameras[entity->derivedId];
        case Entity_PointLight: return (void*)&man->pointLights[entity->derivedId];
    }
    
    return nullptr;
}

EntityKey GetKey(EntityManager* man, Entity* entity)
{
    if(!entity) return NullKey();
    
    u32 id = (u32)(entity - man->bases.ptr);
    u16 gen = entity->gen;
    return {id, gen};
}

u32 GetId(EntityManager* man, Entity* entity)
{
    if(!entity) return (u32)-1;
    
    u32 id = (u32)(entity - man->bases.ptr);
    return id;
}

Entity* GetMount(EntityManager* man, Entity* entity)
{
    if(!entity) return nullptr;
    return GetEntity(man, entity->mount);
}

void MountEntity(EntityManager* man, Entity* entity, Entity* mountTo)
{
    if(!GetMount(man, entity) && !mountTo) return;
    
    // When mounting an entity, the transform
    // needs to be adjusted as to not have
    // any noticeable changes.
    
    // First, get the current world transform of both entities
    Mat4 worldEntity = ComputeWorldTransform(man, entity);
    if(!mountTo)
    {
        PosRotScaleFromMat4(worldEntity, &entity->pos, &entity->rot, &entity->scale);
        entity->mount = NullKey();
        return;
    }
    
    Mat4 worldMountTo = ComputeWorldTransform(man, mountTo);
    
    // Then turn it into a relative transform
    // with respect to the mounted entity
    worldEntity = ComputeTransformInverse(worldMountTo) * worldEntity;
    PosRotScaleFromMat4(worldEntity, &entity->pos, &entity->rot, &entity->scale);
    
    entity->mount = GetKey(man, mountTo);
}

Mat4 ComputeWorldTransform(EntityManager* man, Entity* entity)
{
    if(!entity) return Mat4::identity;
    
    Mat4 worldTransform = Mat4FromPosRotScale(entity->pos, entity->rot, entity->scale);
    
    Entity* mount = GetMount(man, entity);
    while(mount)
    {
        Mat4 transform = Mat4FromPosRotScale(mount->pos, mount->rot, mount->scale);
        worldTransform = transform * worldTransform;
        
        mount = GetMount(man, mount);
    }
    
    return worldTransform;
}

Mat4 ConvertToLocalTransform(EntityManager* man, Entity* entity, Mat4 world)
{
    if(!entity) return world;
    
    Mat4 mountTransform = ComputeWorldTransform(man, GetMount(man, entity));
    return ComputeTransformInverse(mountTransform) * world;
}

Entity* NewEntity(EntityManager* man)
{
    u32 freeSlot = 0;
    if(man->freeBases.len > 0)
    {
        freeSlot = man->freeBases[man->freeBases.len - 1];
        --man->freeBases.len;
        
        // Wipe out everything except for the generation
        u32 gen = man->bases[freeSlot].gen;
        man->bases[freeSlot] = {0};
        man->bases[freeSlot].gen = gen;
    }
    else
    {
        Append(&man->bases, {0});
        freeSlot = man->bases.len - 1;
    }
    
    Entity* entity = &man->bases[freeSlot];
    entity->pos = {0};
    entity->rot = Quat::identity;
    entity->scale = {.x=1.0f, .y=1.0f, .z=1.0f};
    entity->mount = NullKey();
    entity->mesh     = {};
    entity->material = {};
    return entity;
}

// Default constructor for a given derived type
template<typename t>
t* NewEntity(EntityManager* man)
{
    if constexpr (std::is_same_v<t, Entity>)
    {
        return NewEntity(man);
    }
    else
    {
        auto base = NewEntity(man);
        
        Array<t>* array = GetArrayFromType<t>(man);
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
}

void CommitDestroy(EntityManager* man)
{
    for(u32 i = 0; i < (u32)man->bases.len; ++i)
    {
        Entity* ent = &man->bases[i];
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
                
                mount = GetMount(man, mount);
            }
        }
        
        if(toBeDestroyed)
        {
            // Destroy this entity completely (add it to the freeBases array)
            Append(&man->freeBases, entId);
            ent->flags |= EntityFlags_Destroyed;
            
            // This nullifies all references to this entity
            ++ent->gen;
            
            // TODO: @leak Remove entity from derived type array as well
        }
    }
    
}

// Entity iteration
Entity* FirstLive(EntityManager* man)
{
    u32 id = 0;
    while(id < (u32)man->bases.len && (man->bases[id].flags & EntityFlags_Destroyed))
        ++id;
    
    if(id >= (u32)man->bases.len) return nullptr;
    
    return &man->bases[id];
}

Entity* NextLive(EntityManager* man, Entity* current)
{
    if(!current) return nullptr;
    
    u32 id = GetKey(man, current).id + 1;
    while(id < (u32)man->bases.len && (man->bases[id].flags & EntityFlags_Destroyed))
        ++id;
    
    if(id >= (u32)man->bases.len) return nullptr;
    
    return &man->bases[id];
}

template<typename t>
t* FirstDerivedLive(EntityManager* man)
{
    u32 id = 0;
    Array<t>* array = GetArrayFromType<t>(man);
    while(id < (u32)array->len && ((*array)[id].base->flags & EntityFlags_Destroyed))
        ++id;
    
    if(id >= (u32)array->len) return nullptr;
    
    return array->ptr + id;
}

template<typename t>
t* NextDerivedLive(EntityManager* man, t* current)
{
    u32 id = GetKey(man, current->base).id + 1;
    Array<t>* array = GetArrayFromType<t>(man);
    while(id < (u32)array->len && ((*array)[id].base->flags & EntityFlags_Destroyed))
        ++id;
    
    if(id >= (u32)array->len) return nullptr;
    
    return array->ptr + id;
}

Slice<Slice<Entity*>> ComputeAllLiveChildrenForEachEntity(EntityManager* man, Arena* dst)
{
    auto resPtr = ArenaZAllocArray(Slice<Entity*>, man->bases.len, dst);
    Slice<Slice<Entity*>> res = {.ptr = resPtr, .len = man->bases.len };
    
    ScratchArena scratch;
    auto ptr = ArenaZAllocArray(Array<Entity*>, man->bases.len, dst);
    Slice<Array<Entity*>> childrenPerEntity = {.ptr = ptr, .len = man->bases.len };
    defer
    {
        for(int i = 0; i < childrenPerEntity.len; ++i)
            Free(&childrenPerEntity[i]);
    };
    
    for_live_entities(man, ent)
    {
        Entity* mount = GetMount(man, ent);
        if(mount && !(mount->flags & EntityFlags_Destroyed))
        {
            u32 id = GetId(man, mount);
            Append(&childrenPerEntity[id], ent);
        }
    }
    
    for(int i = 0; i < childrenPerEntity.len; ++i)
        res[i] = CopyToArena(&childrenPerEntity[i], dst);
    return res;
}

bool IsChild(EntityManager* man, Entity* suspectedChild, Entity* entity)
{
    Slice<Entity*> children = man->liveChildrenPerEntity[GetId(man, entity)];
    for(int i = 0; i < children.len; ++i)
    {
        if(suspectedChild == children[i] || IsChild(man, suspectedChild, children[i]))
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
