
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
    
    float pos = 0.0f;
    
    Entity* e[1000];
    for(int i = 0; i < 10; ++i)
    {
        e[i] = NewEntity();
        e[i]->model = raptoidModel;
        e[i]->pos.x = pos;
        pos += 3.0f;
    }
    
    e[4]->pos.x = 2.0f;
    MountEntity(e[4], e[3]);
    e[5]->pos.x = 2.0f;
    MountEntity(e[5], e[4]);
    e[6]->pos.x = 2.0f;
    MountEntity(e[6], e[5]);
    
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

void MainUpdate(Entities* entities, UIState* ui, float deltaTime, Arena* permArena, Arena* frameArena)
{
    OS_DearImguiBeginFrame();
    
    PollAndProcessInput();
    
    UpdateUI(ui);
    
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
    
    Camera* mainCamera = GetMainCamera();
    if(mainCamera)
        UpdateMainCamera(mainCamera, deltaTime);
}

void RenderEntities(Entities* entities, float deltaTime)
{
    Camera* camera = GetMainCamera();
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

Slice<Slice<Entity*>> ComputeAllChildrenForEachEntity(Arena* dst)
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
    
    for(int i = 0; i < entities.bases.len; ++i)
    {
        Entity* e = &entities.bases[i];
        Entity* mount = GetMount(e);
        if(mount)
        {
            u32 id = GetId(mount);
            Append(&childrenPerEntity[id], e);
        }
    }
    
    for(int i = 0; i < childrenPerEntity.len; ++i)
        res[i] = CopyToArena(&childrenPerEntity[i], dst);
    return res;
}

void UpdateMainCamera(Camera* camera, float deltaTime)
{
#if 1
    Input input = GetInput();
    
    // Camera rotation
    const float rotateXSpeed = Deg2Rad(120);
    const float rotateYSpeed = Deg2Rad(80);
    const float mouseSensitivity = Deg2Rad(0.08f);  // Degrees per pixel
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
        keyboardX = (float)(input.virtualKeys[Keycode_D] - input.virtualKeys[Keycode_A]);
        keyboardY = (float)(input.virtualKeys[Keycode_W] - input.virtualKeys[Keycode_S]);
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
        keyboardX = (float)(input.virtualKeys[Keycode_D] - input.virtualKeys[Keycode_A]);
        keyboardY = (float)(input.virtualKeys[Keycode_W] - input.virtualKeys[Keycode_S]);
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

UIState InitUI()
{
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // Setup behavior flags
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
    float dpi = OS_GetDPIScale();
    
    // Setup style
    {
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.10f, 0.96f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
        colors[ImGuiCol_Border]                 = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
        colors[ImGuiCol_Button]                 = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
        colors[ImGuiCol_Separator]              = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_PlotLines]              = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram]          = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_DragDropTarget]         = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);
        colors[ImGuiCol_DockingPreview]         = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
        colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowPadding                     = ImVec2(8.00f, 8.00f);
        style.FramePadding                      = ImVec2(5.00f, 4.00f);
        style.CellPadding                       = ImVec2(6.50f, 6.50f);
        style.ItemSpacing                       = ImVec2(6.00f, 6.00f);
        style.ItemInnerSpacing                  = ImVec2(6.00f, 6.00f);
        style.TouchExtraPadding                 = ImVec2(0.00f, 0.00f);
        style.IndentSpacing                     = 25;
        style.ScrollbarSize                     = 15;
        style.GrabMinSize                       = 10;
        style.WindowBorderSize                  = 1;
        style.ChildBorderSize                   = 1;
        style.PopupBorderSize                   = 1;
        style.FrameBorderSize                   = 1;
        style.TabBorderSize                     = 1;
        style.WindowRounding                    = 7;
        style.ChildRounding                     = 4;
        style.FrameRounding                     = 3;
        style.PopupRounding                     = 4;
        style.ScrollbarRounding                 = 9;
        style.GrabRounding                      = 3;
        style.LogSliderDeadzone                 = 4;
        style.TabRounding                       = 4;
        
        style.ScaleAllSizes(dpi);
    }
    
    // Setup font
    {
        ImFont* mainFont = io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 15.0f * dpi);
    }
    
    // Setup UI State
    UIState state = {0};
    state.entityListWindowOpen = true;
    state.propertyWindowOpen = true;
    return state;
}

void UpdateUI(UIState* ui)
{
    ImGuiDockNodeFlags flags = ImGuiDockNodeFlags_PassthruCentralNode;
    
    ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(0, 0, flags);
    
    ShowMainMenuBar(ui);
    
    if(ui->entityListWindowOpen)
    {
        ShowEntityList(ui);
    }
    
    if(ui->propertyWindowOpen)
    {
        ImGui::Begin("Properties");
        defer { ImGui::End(); };
        
        ImGui::Text("Some introspection stuff here i guess");
    }
    
    if(ui->metricsWindowOpen)
    {
        ImGui::ShowMetricsWindow(&ui->metricsWindowOpen);
    }
    
    static bool open = true;
    if(open)
        ImGui::ShowDemoWindow(&open);
}

void ShowMainMenuBar(UIState* ui)
{
    if (ImGui::BeginMainMenuBar())
    {
        if(ImGui::BeginMenu("File"))
        {
            if(ImGui::MenuItem("New", "CTRL+N")) {}
            
            ImGui::EndMenu();
        }
        
        if(ImGui::BeginMenu("Edit"))
        {
            if(ImGui::MenuItem("Undo", "CTRL+Z")) {}
            if(ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
            ImGui::Separator();
            if(ImGui::MenuItem("Cut", "CTRL+X")) {}
            if(ImGui::MenuItem("Copy", "CTRL+C")) {}
            if(ImGui::MenuItem("Paste", "CTRL+V")) {}
            ImGui::EndMenu();
        }
        
        if(ImGui::BeginMenu("Window"))
        {
            if(ImGui::MenuItem("Entities", "", ui->entityListWindowOpen, true))
                ui->entityListWindowOpen ^= true;
            if(ImGui::MenuItem("Properties", "", ui->propertyWindowOpen, true))
                ui->propertyWindowOpen ^= true;
            if(ImGui::MenuItem("Metrics", "", ui->metricsWindowOpen, true))
                ui->metricsWindowOpen ^= true;
            
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

void ShowEntityList(UIState* ui)
{
    ScratchArena scratch;
    Slice<Slice<Entity*>> childrenPerEntity = ComputeAllChildrenForEachEntity(scratch);
    
    ImGui::Begin("Entities");
    defer { ImGui::End(); };
    
    for_live_entities(e)
    {
        const char* kindStr = "";
        switch(e->derivedKind)
        {
            case Entity_Count: break;
            case Entity_None: kindStr = "Base"; break;
            case Entity_Player: kindStr = "Player"; break;
            case Entity_Camera: kindStr = "Camera"; break;
            case Entity_PointLight: kindStr = "PointLight"; break;
        }
        
        if(!GetMount(e))
        {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
            bool isLeaf = false;
            if(childrenPerEntity[GetId(e)].len == 0)
            {
                flags |= ImGuiTreeNodeFlags_Leaf;
                isLeaf = true;
            }
            
            ImGui::PushID(GetId(e));
            bool pushed = ImGui::TreeNodeEx(kindStr, flags);
            ImGui::PopID();
            
            if(pushed)
            {
                defer { ImGui::TreePop(); };
                ShowEntityChildren(ui, e, childrenPerEntity);
            }
        }
    }
}

void ShowEntityChildren(UIState* ui, Entity* entity, Slice<Slice<Entity*>> childrenPerEntity)
{
    Slice<Entity*> children = childrenPerEntity[GetId(entity)];
    for(int i = 0; i < children.len; ++i)
    {
        Entity* e = children[i];
        
        const char* kindStr = "";
        switch(e->derivedKind)
        {
            case Entity_Count: break;
            case Entity_None: kindStr = "Base"; break;
            case Entity_Player: kindStr = "Player"; break;
            case Entity_Camera: kindStr = "Camera"; break;
            case Entity_PointLight: kindStr = "PointLight"; break;
        }
        
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
        bool isLeaf = false;
        if(childrenPerEntity[GetId(e)].len == 0)
        {
            flags |= ImGuiTreeNodeFlags_Leaf;
            isLeaf = true;
        }
        
        u32 id = GetId(e);
        ImGui::PushID(id);
        bool pushed = ImGui::TreeNodeEx(kindStr, flags);
        //if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        //node_clicked = i;
        ImGui::PopID();
        
        if(pushed)
        {
            defer { ImGui::TreePop(); };
            ShowEntityChildren(ui, e, childrenPerEntity);
        }
    }
}
