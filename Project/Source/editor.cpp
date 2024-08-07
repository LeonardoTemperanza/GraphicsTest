
#include "editor.h"
#include "imgui/imgui_internal.h"
#include "generated_meta.h"

// Need it for gizmo intersection
#include "collision.h"

// TODO: @tmp Use something else later
#include <unordered_map>
std::unordered_map<ImGuiID, Widget> widgetTable;
u64 Widget::frameCounter = 0;

Console InitConsole()
{
    Console res = {0};
    
    ClearLog();
    memset(res.inputBuf, 0, sizeof(res.inputBuf));
    res.historyPos = -1;
    
    // "CLASSIFY" is here to provide the test case where "C"+[tab] completes to "CL" and display multiple matches.
    Append(&res.commands, "HELP");
    Append(&res.commands, "HISTORY");
    Append(&res.commands, "CLEAR");
    Append(&res.commands, "CLASSIFY");
    res.autoScroll = true;
    res.scrollToBottom = false;
    return res;
}

static Console console = InitConsole();

Editor InitEditor()
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
        colors[ImGuiCol_TitleBg]                = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
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
    Editor state = {0};
    state.entityListWindowOpen = true;
    state.propertyWindowOpen = true;
    state.camParams.fov = 90.0f;
    state.camParams.nearClip = 0.1f;
    state.camParams.farClip = 1000.0f;
    state.camRot = Quat::identity;
    
#ifdef Development
    state.inEditor = true;
#endif
    
    // Rendering
    state.selectedFramebuffer = R_CreateFramebuffer(0, 0, true, R_TexR8UI, true, false);
    state.entityIdFramebuffer = R_CreateFramebuffer(0, 0, true, R_TexR32I, true, false);
    R_Shader screenVert = LoadShader("CompiledShaders/screenspace_vertex.shader");
    R_Shader paintTrue = LoadShader("CompiledShaders/paint_bool_true.shader");
    R_Shader paintInt = LoadShader("CompiledShaders/paint_int.shader");
    R_Shader model2Proj = LoadShader("CompiledShaders/model2proj.shader");
    R_Shader outlineFrag = LoadShader("CompiledShaders/outline_from_int_texture.shader");
    R_Shader paintTrueShaders[] = {model2Proj, paintTrue};
    state.paintTruePipeline = R_CreatePipeline(ArrToSlice(paintTrueShaders));
    R_Shader outlineShaders[] = {screenVert, outlineFrag};
    state.outlinePipeline = R_CreatePipeline(ArrToSlice(outlineShaders));
    return state;
}

void UpdateEditor(Editor* ui, float deltaTime)
{
    Input input = GetInput();
    ImGuiIO& imguiIo = ImGui::GetIO();
    
    // Prune widgets which weren't used last frame
    RemoveUnusedWidgets();
    
    // Hide mouse cursor if right clicking on main window
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
    
    // This makes it so we lose focus when right clicking as well
    if (!imguiIo.WantCaptureMouse)
    {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
            ImGui::SetWindowFocus(nullptr);
    }
    
    ImGuiDockNodeFlags flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(0, 0, flags);
    
    ShowMainMenuBar(ui);
    
    // Shortcuts
    {
        if(input.unfilteredKeys[Keycode_Ctrl])
        {
            if(PressedUnfilteredKey(input, Keycode_P))
            {
                ui->inEditor = false;
            }
        }
    }
    
    // Editor Camera
    {
        UpdateEditorCamera(ui, deltaTime);
    }
    
    // Gizmos handling
    // This should consume inputs, so that the main editor
    // stuff such as clicking on entities does not take
    // precedence over this
    bool isInteractingWithGizmos = false;
    {
        if(ui->selected.len == 1)
        {
            Entity* selected = ui->selected[0];
            
            Mat4 world = ComputeWorldTransform(selected);
            Vec3 pos, scale;
            Quat rot;
            PosRotScaleFromMat4(world, &pos, &rot, &scale);
            isInteractingWithGizmos = TranslationGizmo("EntityTranslate", ui, &selected->pos);
            Mat4 newWorld = Mat4FromPosRotScale(pos, rot, scale);
            ConvertToLocalTransform(selected, newWorld);
            PosRotScaleFromMat4(newWorld, &pos, &rot, &scale);
            //selected->pos = pos;
        }
    }
    
    // Main editor stuff
    {
        if(!isInteractingWithGizmos && PressedKey(input, Keycode_LMouse))
        {
            // TODO: @tmp Uncomment this
#if 0
            int picked = R_ReadMousePickId((int)input.mouseX, (int)input.mouseY);
            if(picked != -1)
            {
                SelectEntity(ui, GetEntity(picked));
            }
            else
                Free(&ui->selected);
#endif
        }
    }
    
    // Entity list window
    if(ui->entityListWindowOpen)
    {
        ShowEntityList(ui);
    }
    
    // Property window
    if(ui->propertyWindowOpen)
    {
        ImGui::Begin("Properties");
        
        if(ui->selected.len == 1)
        {
            Entity* selected = ui->selected[0];
            ShowEntityControl(ui, selected);
        }
        else if(ui->selected.len == 0)
        {
            ImGui::Text("No entity selected.\n");
        }
        else
        {
            ImGui::Text("Multiple entities are selected. Multiselection editing is currently not supported.\n");
        }
        
        ImGui::End();
    }
    
    // Metrics window
    if(ui->metricsWindowOpen)
    {
        ImGui::ShowMetricsWindow(&ui->metricsWindowOpen);
    }
    
    // Console window
    {
        Input input = GetInput();
        if(input.unfilteredKeys[Keycode_Insert] && !input.prev.unfilteredKeys[Keycode_Insert])
        {
            ui->consoleOpen ^= true;
            // Set window focus
            if(ui->consoleOpen)
            {
                ImGui::SetWindowFocus("Console");
                ui->focusOnConsole = true;
            }
            else
                ImGui::SetWindowFocus(nullptr);
        }
        
        defer { ui->focusOnConsole = false; };
        
        const float smoothing = 0.000000001f;
        ui->consoleAnimation = ApproachExponential(ui->consoleAnimation, ui->consoleOpen, smoothing, deltaTime); 
        
        const float consoleHeight = 1000.0f;
        const float consoleWidth  = 1000.0f;
        
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, lerp(-1000.0f, 0.0f, ui->consoleAnimation)), ImGuiCond_Always, ImVec2(0.5f,0.0f));
        ImGui::SetNextWindowSize(ImVec2(consoleWidth, consoleHeight));
        ShowConsole(ui);
    }
    
    // Debug window
    if(ui->debugWindowOpen)
    {
        ImGui::Begin("Debugging", &ui->debugWindowOpen);
        ImGui::End();
    }
    
    // Camera settings window
    if(ui->cameraSettingsWindowOpen)
    {
        ImGui::Begin("Editor Camera", &ui->cameraSettingsWindowOpen);
        ImGui::DragFloat("Horizontal FOV", &ui->camParams.fov, 0.05f);
        ImGui::End();
    }
    
    // ImGui Demo window
    if(ui->demoWindowOpen)
        ImGui::ShowDemoWindow(&ui->demoWindowOpen);
    
    // Stats window
    if(ui->statsWindowOpen)
    {
        ImGui::Begin("Stats", &ui->statsWindowOpen);
        ImGui::End();
    }
}

void RenderEditor(Editor* editor)
{
    int width, height;
    OS_GetClientAreaSize(&width, &height);
    R_ResizeFramebuffer(editor->selectedFramebuffer, width, height);
    R_ResizeFramebuffer(editor->entityIdFramebuffer, width, height);
    
    // Render pass for selected entities
    {
        R_SetFramebuffer(editor->selectedFramebuffer);
        R_SetPipeline(editor->paintTruePipeline);
        R_ClearFrame({0});
        
        for(int i = 0; i < editor->selected.len; ++i)
        {
            Entity* ent = editor->selected[i];
            if(ent->flags & EntityFlags_Destroyed) continue;
            
            R_UniformValue perObjValues[] =
            {
                MakeUniformMat4(transpose(ComputeWorldTransform(ent)))
            };
            R_UploadUniformBuffer(GetPerObjUniformBuffer(), ArrToSlice(perObjValues));
            R_DrawModel(ent->model);
        }
        
        R_SetFramebuffer(R_DefaultFramebuffer());
    }
    
    // Render pass for entity ids (clicking on entities to select them)
#if 0
    {
        R_SetFramebuffer(editor->selectedFramebuffer);
        R_SetPipeline(editor->paintIdPipeline);
        R_ClearFrameInt(-1, -1, -1, -1);
        
        for(int i = 0; i < editor->selected.len; ++i)
        {
            Entity* ent = editor->selected[i];
            if(ent->flags & EntityFlags_Destroyed) continue;
            
            R_UniformValue perObjValues[] =
            {
                MakeUniformMat4(transpose(ComputeWorldTransform(ent)))
            };
            R_UploadUniformBuffer(GetPerObjUniformBuffer(), ArrToSlice(perObjValues));
            
            // Upload the int uniform here
            
            R_DrawModel(ent->model);
        }
        
        R_SetFramebuffer(R_DefaultFramebuffer());
    }
#endif
    
    // Draw outline of selected objects
    {
        R_EnableDepthTest(false);
        R_EnableAlphaBlending(true);
        R_SetTexture(R_GetFramebufferColorTexture(editor->selectedFramebuffer), 0);
        R_SetPipeline(editor->outlinePipeline);
        R_DrawFullscreenQuad();
        R_EnableDepthTest(true);
    }
}

void ShowMainMenuBar(Editor* ui)
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
            if(ImGui::MenuItem("DearImgui Demo", "", ui->demoWindowOpen, true))
                ui->demoWindowOpen ^= true;
            if(ImGui::MenuItem("Debugging", "", ui->debugWindowOpen, true))
                ui->debugWindowOpen ^= true;
            if(ImGui::MenuItem("Stats", "", ui->statsWindowOpen, true))
                ui->statsWindowOpen ^= true;
            if(ImGui::MenuItem("Editor Camera Settings", "", ui->cameraSettingsWindowOpen, true))
                ui->cameraSettingsWindowOpen ^= true;
            
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

void ShowEntityList(Editor* ui)
{
    ScratchArena scratch;
    Slice<Slice<Entity*>> childrenPerEntity = ComputeAllChildrenForEachEntity(scratch);
    
    ImGui::Begin("Entities");
    defer { ImGui::End(); };
    
    // Construct query
    ImGui::SeparatorText("Query parameters");
    if(ImGui::Button("Add##QueryParameters"))
        ImGui::OpenPopup("Combo##QueryParameters");
    
    if(ImGui::BeginPopup("Combo##QueryParameters"))
    {
        const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIIIIII", "JJJJ", "KKKKKKK" };
        static int item_current = 0;
        ImGui::Combo("##QueryParametersChoice", &item_current, items, IM_ARRAYSIZE(items));
        
        ImGui::EndPopup();
    }
    
    static bool isStatic = false;
    ImGui::Checkbox("Static", &isStatic);
    
    ImGui::SeparatorText("Entity list");
    
    for_live_entities(e)
    {
        if(!GetMount(e))
            ShowEntityAndChildren(ui, e, childrenPerEntity);
    }
}


void ShowEntityAndChildren(Editor* ui, Entity* entity, Slice<Slice<Entity*>> childrenPerEntity)
{
    u32 id = GetId(entity);
    
    // Show entity tree node
    const char* kindStr = "";
    switch(entity->derivedKind)
    {
        case Entity_Count: break;
        case Entity_None: kindStr = "Base"; break;
        case Entity_Player: kindStr = "Player"; break;
        case Entity_Camera: kindStr = "Camera"; break;
        case Entity_PointLight: kindStr = "PointLight"; break;
    }
    
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    bool isLeaf = false;
    if(childrenPerEntity[id].len == 0)
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
        isLeaf = true;
    }
    
    int selectionId = GetEntitySelectionId(ui, entity);
    bool isSelected = selectionId > -1;
    if(isSelected)
        flags |= ImGuiTreeNodeFlags_Selected;
    
    ImGui::PushID(id);
    bool nodeOpen = ImGui::TreeNodeEx(kindStr, flags);
    ImGui::PopID();
    
    if(ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        SelectEntity(ui, entity);
    }
    
    // Drag and drop
    if(ImGui::BeginDragDropTarget())
    {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EntityListNode");
        if(payload)
        {
            Entity* draggedFrom = *(Entity**)payload->Data;
            if(draggedFrom && !IsChild(entity, draggedFrom, childrenPerEntity))
            {
                MountEntity(draggedFrom, entity);
            }
        }
        
        ImGui::EndDragDropTarget();
    }
    
    if(ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("EntityListNode", &entity, sizeof(Entity*));
        ImGui::EndDragDropSource();
    }
    
    // Right click options
    if(GetMount(entity) && ImGui::BeginPopupContextWindow())
    {
        if(ImGui::Selectable("Break mount"))
            MountEntity(entity, nullptr);
        
        ImGui::EndPopup();
    }
    
    // Children visualization
    if(nodeOpen)
    {
        Slice<Entity*> children = childrenPerEntity[GetId(entity)];
        for(int i = 0; i < children.len; ++i)
        {
            Entity* e = children[i];
            
            ShowEntityAndChildren(ui, e, childrenPerEntity);
        }
        
        ImGui::TreePop();
    }
}

void UpdateEditorCamera(Editor* editor, float deltaTime)
{
    Input input = GetInput();
    
    Vec3& pos = editor->camPos;
    Quat& rot = editor->camRot;
    
    // Camera rotation
    const float rotateXSpeed = Deg2Rad(120);
    const float rotateYSpeed = Deg2Rad(80);
    const float mouseSensitivity = Deg2Rad(0.2f);  // Degrees per pixel
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
    rot = xRot * yRot;
    
    // Camera position
    static Vec3 curVel = {0};
    
    const float moveSpeed = 4.0f;
    const float moveAccel = 30.0f;
    
    float keyboardX = 0.0f;
    float keyboardY = 0.0f;
    float keyboardZ = 0.0f;
    if(input.virtualKeys[Keycode_RMouse])
    {
        keyboardX = (float)(input.virtualKeys[Keycode_D] - input.virtualKeys[Keycode_A]);
        keyboardY = (float)(input.virtualKeys[Keycode_E] - input.virtualKeys[Keycode_Q]);
        keyboardZ = (float)(input.virtualKeys[Keycode_W] - input.virtualKeys[Keycode_S]);
    }
    
    Vec3 targetVel =
    {
        .x = (input.gamepad.leftStick.x + keyboardX) * moveSpeed,
        .y = 0.0f,
        .z = (input.gamepad.leftStick.y + keyboardZ) * moveSpeed
    };
    
    if(dot(targetVel, targetVel) > moveSpeed * moveSpeed)
        targetVel = normalize(targetVel) * moveSpeed;
    
    targetVel = rot * targetVel;
    targetVel.y += (input.gamepad.rightTrigger - input.gamepad.leftTrigger) * moveSpeed;
    targetVel.y += keyboardY * moveSpeed;
    
    curVel = ApproachLinear(curVel, targetVel, moveAccel * deltaTime);
    pos += curVel * deltaTime;
}

int GetEntitySelectionId(Editor* ui, Entity* entity)
{
    for(int i = 0; i < ui->selected.len; ++i)
    {
        if(ui->selected[i] == entity)
            return i;
    }
    
    return -1;
}

void SelectEntity(Editor* ui, Entity* entity)
{
    Input input = GetInput();
    bool holdingCtrl = input.unfilteredKeys[Keycode_Ctrl];
    if(!holdingCtrl)
        Free(&ui->selected);
    
    int selectionId = GetEntitySelectionId(ui, entity);
    bool isSelected = selectionId > -1;
    if(holdingCtrl && isSelected)
    {
        ui->selected[selectionId] = ui->selected[ui->selected.len - 1];
        --ui->selected.len;
    }
    else
        Append(&ui->selected, entity);
}

// TODO: @tmp This is temporary, it's largely from the dear imgui demo debug window

// Portable helpers
static int Stricmp(const char* s1, const char* s2)
{
    int d;
    while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1)
    {
        s1++;
        s2++;
    }
    
    return d;
}

static int Strnicmp(const char* s1, const char* s2, int n)
{
    int d = 0;
    while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1)
    {
        s1++;
        s2++;
        n--;
    }
    
    return d;
}

static char* Strdup(const char* s)
{
    IM_ASSERT(s);
    size_t len = strlen(s) + 1;
    void* buf = ImGui::MemAlloc(len);
    IM_ASSERT(buf);
    return (char*)memcpy(buf, (const void*)s, len);
}

static void Strtrim(char* s)
{
    char* str_end = s + strlen(s);
    while(str_end > s && str_end[-1] == ' ')
        str_end--;
    
    *str_end = 0;
}

void ClearLog()
{
    for(int i = 0; i < console.items.len; i++)
        ImGui::MemFree(console.items[i]);
    Free(&console.items);
}

void Log(const char* fmt, ...)
{
    // FIXME-OPT
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    buf[IM_ARRAYSIZE(buf)-1] = 0;
    va_end(args);
    Append(&console.items, Strdup(buf));
}

void ExecuteCommand(const char* command)
{
    Log("# %s\n", command);
    
    // Process command
    if (Stricmp(command, "CLEAR") == 0)
    {
        ClearLog();
    }
    else if (Stricmp(command, "HELP") == 0)
    {
        Log("Commands:");
        for(int i = 0; i < console.commands.len; i++)
            Log("- %s", console.commands[i]);
    }
    else if (Stricmp(command, "HISTORY") == 0)
    {
        int first = console.history.len - 10;
        for (int i = first > 0 ? first : 0; i < console.history.len; i++)
            Log("%3d: %s\n", i, console.history[i]);
    }
    else
    {
        Log("Unknown command: '%s'\n", command);
    }
    
    // On command input, we scroll to bottom even if AutoScroll==false
    console.scrollToBottom = true;
}

int TextEditCallback(ImGuiInputTextCallbackData* data)
{
    //Log("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
    switch(data->EventFlag)
    {
        case ImGuiInputTextFlags_CallbackCompletion:
        {
            // Example of TEXT COMPLETION
            
            // Locate beginning of current word
            const char* word_end = data->Buf + data->CursorPos;
            const char* word_start = word_end;
            while (word_start > data->Buf)
            {
                const char c = word_start[-1];
                if (c == ' ' || c == '\t' || c == ',' || c == ';')
                    break;
                word_start--;
            }
            
            // Build a list of candidates
            ImVector<const char*> candidates;
            for (int i = 0; i < console.commands.len; i++)
                if (Strnicmp(console.commands[i], word_start, (int)(word_end - word_start)) == 0)
                candidates.push_back(console.commands[i]);
            
            if (candidates.Size == 0)
            {
                // No match
                Log("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
            }
            else if (candidates.Size == 1)
            {
                // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
                data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
                data->InsertChars(data->CursorPos, candidates[0]);
                data->InsertChars(data->CursorPos, " ");
            }
            else
            {
                // Multiple matches. Complete as much as we can..
                // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
                int match_len = (int)(word_end - word_start);
                for (;;)
                {
                    int c = 0;
                    bool all_candidates_matches = true;
                    for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
                        if (i == 0)
                        c = toupper(candidates[i][match_len]);
                    else if (c == 0 || c != toupper(candidates[i][match_len]))
                        all_candidates_matches = false;
                    if (!all_candidates_matches)
                        break;
                    match_len++;
                }
                
                if (match_len > 0)
                {
                    data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
                    data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
                }
                
                // List matches
                Log("Possible matches:\n");
                for (int i = 0; i < candidates.Size; i++)
                    Log("- %s\n", candidates[i]);
            }
            
            break;
        }
        case ImGuiInputTextFlags_CallbackHistory:
        {
            // Example of HISTORY
            const int prev_history_pos = console.historyPos;
            if (data->EventKey == ImGuiKey_UpArrow)
            {
                if (console.historyPos == -1)
                    console.historyPos = console.history.len - 1;
                else if (console.historyPos > 0)
                    console.historyPos--;
            }
            else if (data->EventKey == ImGuiKey_DownArrow)
            {
                if (console.historyPos != -1)
                    if (++console.historyPos >= console.history.len)
                    console.historyPos = -1;
            }
            
            // A better implementation would preserve the data on the current input line along with cursor position.
            if (prev_history_pos != console.historyPos)
            {
                const char* history_str = (console.historyPos >= 0) ? console.history[console.historyPos] : "";
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, history_str);
            }
        }
    }
    return 0;
}

void ShowConsole(Editor* ui)
{
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    if (!ImGui::Begin("Console", nullptr, flags))
    {
        ImGui::End();
        return;
    }
    
    // TODO: display items starting from the bottom
    
    if (ImGui::SmallButton("Add Debug Text"))  { Log("%d some text", console.items.len); Log("some more text"); Log("display very important message here!"); }
    ImGui::SameLine();
    if (ImGui::SmallButton("Add Debug Error")) { Log("[error] something went wrong"); }
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear"))           { ClearLog(); }
    ImGui::SameLine();
    bool copy_to_clipboard = ImGui::SmallButton("Copy");
    //static float t = 0.0f; if (ImGui::GetTime() - t > 0.02f) { t = ImGui::GetTime(); Log("Spam %f", t); }
    
    ImGui::Separator();
    
    // Options menu
    if (ImGui::BeginPopup("Options"))
    {
        ImGui::Checkbox("Auto-scroll", &console.autoScroll);
        ImGui::EndPopup();
    }
    
    // Options, Filter
    ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_O, ImGuiInputFlags_Tooltip);
    if (ImGui::Button("Options"))
        ImGui::OpenPopup("Options");
    ImGui::SameLine();
    console.filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
    ImGui::Separator();
    
    // Reserve enough left-over height for 1 separator + 1 input text
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), ImGuiChildFlags_NavFlattened))
    {
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::Selectable("Clear")) ClearLog();
            ImGui::EndPopup();
        }
        
        // Display every line as a separate entry so we can change their color or add custom widgets.
        // If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
        // NB- if you have thousands of entries this approach may be too inefficient and may require user-side clipping
        // to only process visible items. The clipper will automatically measure the height of your first item and then
        // "seek" to display only items in the visible area.
        // To use the clipper we can replace your standard loop:
        //      for (int i = 0; i < Items.Size; i++)
        //   With:
        //      ImGuiListClipper clipper;
        //      clipper.Begin(Items.Size);
        //      while (clipper.Step())
        //         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        // - That your items are evenly spaced (same height)
        // - That you have cheap random access to your elements (you can access them given their index,
        //   without processing all the ones before)
        // You cannot this code as-is if a filter is active because it breaks the 'cheap random-access' property.
        // We would need random-access on the post-filtered list.
        // A typical application wanting coarse clipping and filtering may want to pre-compute an array of indices
        // or offsets of items that passed the filtering test, recomputing this array when user changes the filter,
        // and appending newly elements as they are inserted. This is left as a task to the user until we can manage
        // to improve this example code!
        // If your items are of variable height:
        // - Split them into same height items would be simpler and facilitate random-seeking into your list.
        // - Consider using manual call to IsRectVisible() and skipping extraneous decoration from your items.
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
        if (copy_to_clipboard)
            ImGui::LogToClipboard();
        for (int i = 0; i < console.items.len; ++i)
        {
            const char* item = console.items[i];
            
            if (!console.filter.PassFilter(item))
                continue;
            
            // Normally you would store more information in your item than just a string.
            // (e.g. make Items[] an array of structure, store color/type etc.)
            ImVec4 color;
            bool has_color = false;
            if (strstr(item, "[error]")) { color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); has_color = true; }
            else if (strncmp(item, "# ", 2) == 0) { color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); has_color = true; }
            if (has_color)
                ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(item, nullptr);
            if (has_color)
                ImGui::PopStyleColor();
        }
        if (copy_to_clipboard)
            ImGui::LogFinish();
        
        // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
        // Using a scrollbar or mouse-wheel will take away from the bottom edge.
        if (console.scrollToBottom || (console.autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
            ImGui::SetScrollHereY(1.0f);
        console.scrollToBottom = false;
        
        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
    ImGui::Separator();
    
    // Command-line
    bool reclaim_focus = false;
    ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
    
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
    if(ui->focusOnConsole) ImGui::SetKeyboardFocusHere(0);
    if(ImGui::InputText("##ConsoleInput", console.inputBuf, IM_ARRAYSIZE(console.inputBuf), input_text_flags, &TextEditCallback, nullptr))
    {
        char* s = console.inputBuf;
        Strtrim(s);
        if (s[0])
            ExecuteCommand(s);
        strcpy(s, "");
        reclaim_focus = true;
    }
    ImGui::PopItemWidth();
    
    // Auto-focus on window apparition
    ImGui::SetItemDefaultFocus();
    if (reclaim_focus)
        ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
    
    if(console.items.len > 3000)
        ClearLog();
    
    ImGui::End();
}

// From: https://github.com/ocornut/imgui/issues/1831 by volcoma
static void PushMultiItemsWidthsAndLabels(const char* labels[], int components, float w_full)
{
    using namespace ImGui;
    
    ImGuiWindow* window = GetCurrentWindow();
    const ImGuiStyle& style = GImGui->Style;
    if(w_full <= 0.0f)
        w_full = GetContentRegionAvail().x;
    
    const float w_item_one =
        ImMax(1.0f, (w_full - (style.ItemInnerSpacing.x * 2.0f) * (components - 1)) / (float)components) -
        style.ItemInnerSpacing.x;
    for(int i = 0; i < components; i++)
        window->DC.ItemWidthStack.push_back(w_item_one - CalcTextSize(labels[i]).x);
    window->DC.ItemWidth = window->DC.ItemWidthStack.back();
}

// From: https://github.com/ocornut/imgui/issues/1831 by volcoma
bool DragFloatNEx(const char* labels[], float* v, int components, float v_speed, float v_min, float v_max,
                  const char* display_format)
{
    using namespace ImGui;
    
    ImGuiWindow* window = GetCurrentWindow();
    if(window->SkipItems)
        return false;
    
    ImGuiContext& g = *GImGui;
    bool value_changed = false;
    BeginGroup();
    
    PushMultiItemsWidthsAndLabels(labels, components, 0.0f);
    for(int i = 0; i < components; i++)
    {
        PushID(labels[i]);
        PushID(i);
        TextUnformatted(labels[i], FindRenderedTextEnd(labels[i]));
        SameLine();
        value_changed |= DragFloat("", &v[i], v_speed, v_min, v_max, display_format);
        SameLine(0, g.Style.ItemInnerSpacing.x);
        PopID();
        PopID();
        PopItemWidth();
    }
    
    EndGroup();
    
    return value_changed;
}

void ShowEntityControl(Editor* editor, Entity* entity)
{
    ImGui::PushID(GetId(entity));
    
    // Show base entity
    ShowStructControl(metaEntity, entity);
    
    if(entity->derivedKind != Entity_None)
    {
        // Show derived entity
        MetaStruct metaStruct = {0};
        switch(entity->derivedKind)
        {
            case Entity_None: break;
            case Entity_Count: break;
            case Entity_Player: metaStruct = metaPointLight; break;
            case Entity_Camera: metaStruct = metaCamera; break;
            case Entity_PointLight: metaStruct = metaPointLight; break;
        }
        
        ShowStructControl(metaStruct, GetDerivedAddr(entity));
    }
    
    ImGui::PopID();
}

void ShowStructControl(MetaStruct metaStruct, void* address)
{
    ImGui::SeparatorText(metaStruct.cName);
    Slice<MemberDefinition> members = metaStruct.members;
    
    for(int i = 0; i < members.len; ++i)
    {
        MemberDefinition member = members[i];
        MetaType metaType = member.typeInfo.metaType;
        if(metaType == Meta_Unknown) continue;
        
        ImGui::PushID(member.cName);
        ImGui::PushID(i);
        
        switch(metaType)
        {
            case Meta_Unknown: break;
            case Meta_Int:
            {
                ImGui::DragInt(member.cNiceName, (int*)((char*)address + member.offset), 0.05f);
                break;
            }
            case Meta_Float:
            {
                ImGui::DragFloat(member.cNiceName, (float*)((char*)address + member.offset), 0.05f);
                break;
            }
            case Meta_Vec3:
            {
                ShowVec3Control(member.cNiceName, (Vec3*)((char*)address + member.offset), 150.0f, 0.05f);
                break;
            }
            case Meta_Quat:
            {
                ShowQuatControl(member.cNiceName, (Quat*)((char*)address + member.offset), 150.0f, 0.5f);
                break;
            }
            //Meta_RecursiveCases
        }
        
        ImGui::PopID();
        ImGui::PopID();
    }
}

void ShowVec3Control(const char* strId, Vec3* value, float columnWidth, float sensitivity)
{
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text(strId);
    ImGui::NextColumn();
    
    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    
    float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    
    ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
    if(ImGui::ButtonEx("X", buttonSize, ImGuiButtonFlags_NoNavFocus))
        value->x = 0.0f;
    ImGui::SameLine();
    
    ImGui::PopStyleColor(3);
    ImGui::PopItemFlag();
    
    ImGui::DragFloat("##X", &value->x, sensitivity);
    ImGui::PopItemWidth();
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.3f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    
    ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
    if(ImGui::Button("Y", buttonSize))
        value->y = 0.0f;
    ImGui::SameLine();
    
    ImGui::PopStyleColor(3);
    ImGui::PopItemFlag();
    
    ImGui::DragFloat("##Y", &value->y, sensitivity);
    ImGui::PopItemWidth();
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    
    ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
    if(ImGui::Button("Z", buttonSize))
        value->z = 0.0f;
    ImGui::SameLine();
    
    ImGui::PopStyleColor(3);
    ImGui::PopItemFlag();
    
    ImGui::DragFloat("##Z", &value->z, sensitivity);
    ImGui::PopItemWidth();
    
    ImGui::PopStyleVar();
    
    ImGui::Columns(1);
}

void ShowQuatControl(const char* strId, Quat* value, float columnWidth, float sensitivity)
{
    // NOTE: As far as i know there's no way to convert to quaternion
    // and then back in a consistent way, so the euler representation
    // needs to be kept between frames.
    
    ImGuiID id = ImGui::GetID("");
    Widget widget = {0};
    widget.eulerAngles = EulerRadToDeg(QuatToEulerRad(*value));
    AddWidgetIfNotPresent(id, widget);
    if(!widgetTable.contains(id))
    {
        Widget widget = {0};
        widgetTable[id] = widget;
    }
    
    widgetTable[id].lastUsedFrame = Widget::frameCounter;
    
    Vec3* euler = &widgetTable[id].eulerAngles;
    ShowVec3Control(strId, euler, columnWidth, sensitivity);
    
    // Convert from euler angles to quaternion
    Vec3 eulerRad = EulerDegToRad(*euler);
    *value = EulerRadToQuat(eulerRad);
}

void RemoveUnusedWidgets()
{
    for(auto it = widgetTable.begin(); it != widgetTable.end();)
    {
        bool removeThis = false;
        
        if(it->second.lastUsedFrame < Widget::frameCounter)
            removeThis = true;
        
        if(removeThis)
            it = widgetTable.erase(it);
        else
            it++;
    }
    
    ++Widget::frameCounter;
}

bool AddWidgetIfNotPresent(ImGuiID id, Widget initialState)
{
    bool res = true;
    if(!widgetTable.contains(id))
    {
        res = false;
        widgetTable[id] = initialState;
    }
    
    widgetTable[id].lastUsedFrame = Widget::frameCounter;
    return res;
}

// TODO: @tmp Quick function that we use for now. Will later be
// pushed to the renderer
void DrawQuickLine(Vec3 v1, Vec3 v2, float scale, Vec4 color)
{
    Vec3 diff = v2 - v1;
    diff = normalize(diff);
    
    // Direction to choose, to put in cross product to get
    // perpendicular direction
    Vec3 dir = Vec3::up;
    if(std::abs(dot(diff, dir)) > 0.8f)
        dir = Vec3::right;
    if(std::abs(dot(diff, dir)) > 0.8f)
        dir = Vec3::forward;
    
    Vec3 perp = cross(diff, dir);
    Vec3 q1 = v1 + perp * scale;
    Vec3 q2 = v1 - perp * scale;
    Vec3 q3 = v2 - perp * scale;
    Vec3 q4 = v2 + perp * scale;
    //R_ImDrawQuad(q1, q2, q3, q4, color);
    //R_ImDrawQuad(q4, q3, q2, q1, color);
}

bool TranslationGizmo(const char* strId, Editor* editor, Vec3* pos)
{
    ImGuiID id = ImGui::GetID(strId);
    Widget initial = {0};
    AddWidgetIfNotPresent(id, initial);
    Widget& widget = widgetTable[id];
    
    Input input = GetInput();
    bool interacting = false;
    bool interactingX = false;
    bool interactingY = false;
    bool interactingZ = false;
    
    // Draw a quad, scaled to be the same size even when further away
    float fov = Deg2Rad(editor->camParams.fov);
    Vec3 diff = *pos - editor->camPos;
    // Get forward distance only
    float dist = dot(diff, editor->camRot * Vec3::forward);
    float screenHeightAtDistance = 2.0f * dist * tan(fov / 2.0f);
    int width, height;
    OS_GetClientAreaSize(&width, &height);
    float pixelsPerUnit = height / screenHeightAtDistance;
    float scale = 125.0f / pixelsPerUnit;
    
    Ray cameraRay = CameraRay((int)input.mouseX, (int)input.mouseY, editor->camPos, editor->camRot, editor->camParams.fov);
    
    Vec4 red    = {1, 0, 0, 1};
    Vec4 green  = {0, 1, 0, 1};
    Vec4 blue   = {0, 0, 1, 1};
    Vec4 yellow = {1, 1, 0, 1};
    
    // X
    if(!interacting)
    {
        interactingX = AxisHandle(cameraRay, pos, Vec3::right, red, scale,
                                  &widget.clickedX, &widget.vecDragStart, &widget.dragStartMousePos);
        interacting |= interactingX;
    }
    
    // Y
    if(!interacting)
    {
        interactingY = AxisHandle(cameraRay, pos, Vec3::up, green, scale,
                                  &widget.clickedY, &widget.vecDragStart, &widget.dragStartMousePos);
        interacting |= interactingY;
    }
    
    // Z
    if(!interacting)
    {
        interactingZ = AxisHandle(cameraRay, pos, Vec3::forward, blue, scale,
                                  &widget.clickedZ, &widget.vecDragStart, &widget.dragStartMousePos);
        interacting |= interactingZ;
    }
    
    // Rendering
    DrawQuickLine(*pos, *pos + Vec3::right * scale, 0.05f * scale, interactingX ? yellow : red);
    DrawQuickLine(*pos, *pos + Vec3::up * scale, 0.05f * scale, interactingY ? yellow : green);
    DrawQuickLine(*pos, *pos + Vec3::forward * scale, 0.05f * scale, interactingZ ? yellow : blue);
    
    return interacting;
}

void RotationGizmo(const char* strId, Quat* rot)
{
    
}

void ScaleGizmo(const char* strId, Vec3* scale)
{
    
}

bool AxisHandle(Ray cameraRay, Vec3* pos, Vec3 dir, Vec4 color, float scale, bool* clicked, Vec3* dragStart, Vec3* dragStartMousePos)
{
    bool interacting = false;
    
    Input input = GetInput();
    bool pressingLMouse = input.virtualKeys[Keycode_LMouse];
    bool pressedLMouse = PressedKey(input, Keycode_LMouse);
    if(*clicked && pressingLMouse)
    {
        interacting = true;
        
        bool ok = true;
        Vec3 projected = ProjectToLine(cameraRay, *pos, dir, &ok);
        if(ok)
            *pos = *dragStart + (projected - *dragStartMousePos);
    }
    else if(*clicked && !pressingLMouse)
    {
        *clicked = false;
    }
    else if(!*clicked && pressedLMouse)
    {
        // TODO: This part doesn't work in general for all directions,
        // only cardinal directions for now
        Vec3 v1 = {0};
        Vec3 v2 = {0};
        Vec3 v3 = {0};
        if(dot(dir, Vec3::up) > 0.8f)
        {
            v1 = Vec3::right;
            v2 = Vec3::forward;
            v3 = Vec3::up;
        }
        else if(dot(dir, Vec3::right) > 0.8f)
        {
            v1 = Vec3::forward;
            v2 = Vec3::up;
            v3 = Vec3::right;
        }
        else if(dot(dir, Vec3::forward) > 0.8f)
        {
            v1 = Vec3::right;
            v2 = Vec3::up;
            v3 = Vec3::forward;
        }
        
        Aabb aabb = {.min=*pos - v1*0.05f*scale - v2*0.05f*scale, .max=*pos + v1*0.05f*scale + v2*0.05f*scale + v3*scale};
        if(RayAabbIntersection(cameraRay, aabb))
        {
            interacting = true;
            
            bool wasClicked = *clicked;
            *clicked = true;
            *dragStart = *pos;
            
            bool ok = true;
            Vec3 projected = ProjectToLine(cameraRay, *pos, dir, &ok);
            if(ok)
                *dragStartMousePos = projected;
            else
                *dragStartMousePos = *pos;
        }
    }
    
    return interacting;
}

Vec3 ProjectToLine(Ray cameraRay, Vec3 pos, Vec3 dir, bool* outSuccess)
{
    Vec3 planeNormal = Vec3::up;
    if(std::abs(dot(dir, planeNormal)) > 0.8f)
        planeNormal = Vec3::right;
    if(std::abs(dot(dir, planeNormal)) > 0.8f)
        planeNormal = Vec3::forward;
    planeNormal = cross(dir, planeNormal);
    
    if(dot(planeNormal, cameraRay.dir) >= 0.0f)
        planeNormal = -planeNormal;
    
    float planeDst = RayPlaneDst(cameraRay, pos, planeNormal);
    if(planeDst != FLT_MAX)
    {
        Vec3 intersectionPoint = cameraRay.ori + cameraRay.dir * planeDst;
        Vec3 projected = dir * dot(intersectionPoint, dir);
        return projected;
    }
    
    *outSuccess = false;
    return pos;
}
