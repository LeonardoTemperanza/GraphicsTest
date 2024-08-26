
#pragma once

// Editor stuff

#include "core.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "generated_meta.h"
#include "collision.h"

// TODO: @tmp Use something else later
#include <unordered_map>

struct QueryElement
{
    EntityFlags flag;
    bool enabled;
};

struct Widget;
struct Editor
{
    // Open windows
    bool entityListWindowOpen;
    bool propertyWindowOpen;
    bool metricsWindowOpen;
    bool consoleOpen;
    bool demoWindowOpen;
    bool statsWindowOpen;
    bool debugWindowOpen;
    bool cameraSettingsWindowOpen;
    bool inEditor;
    
    bool focusOnConsole;
    
    float consoleAnimation;  // 0 when fully hidden, 1 when fully visible
    
    Array<Entity*> selected;
    
    // Rendering
    R_Framebuffer selectedFramebuffer;  // Serves to draw selected objects to
    R_Framebuffer entityIdFramebuffer;  // Serves to draw entity id to (for clicking on entities)
    
    // Entity list query
    Array<QueryElement> queryElements;
    EntityFlags entityQuery;
    
    // Editor camera
    Vec3 camPos;
    Quat camRot;
    CameraParams camParams;
    
    // Gizmo and custom UI state
    
    // @tmp Use something else later, like stb map or simply an array
    std::unordered_map<ImGuiID, Widget> widgetTable;
    
    SceneParams* sceneParams;
};

// TODO: @tmp This is mostly taken from the dear imgui example
struct Console
{
    char               inputBuf[256];
    Array<char*>       items;
    Array<const char*> commands;
    Array<char*>       history;
    int                historyPos;    // -1: new line, 0..History.Size-1 browsing history.
    ImGuiTextFilter    filter;
    bool               autoScroll;
    bool               scrollToBottom;
};

Editor InitEditor();
void UpdateEditor(Editor* editor, float deltaTime);
void RenderEditor(Editor* editor, float deltaTime);
void ShowMainMenuBar(Editor* editor);
void ShowEntityList(Editor* editor);
void ShowEntityAndChildren(Editor* editor, Entity* entity, Slice<Slice<Entity*>> childrenPerEntity);
void UpdateEditorCamera(Editor* editor, float deltaTime);

int GetEntitySelectionId(Editor* ui, Entity* entity);
void SelectEntity(Editor* ui, Entity* entity);

// Console print utilities
void ClearLog();
void Log(const char* fmt, ...);
void ExecuteCommand(const char* command);
int TextEditCallback(ImGuiInputTextCallbackData* data);

void ShowConsole(Editor* ui);

// Introspection
void ShowEntityControl(Editor* editor, Entity* entity);
void ShowStructControl(MetaStruct metaStruct, Editor* editor, void* address);

// Dear Imgui helpers for basic types
void ShowVec3Control(const char* strId, Vec3* value, float columnWidth, float sensitivity);
void ShowQuatControl(const char* strId, Editor* editor, Quat* value, float columnWidth, float sensitivity);

// Info to be stored in the widget table
// where we store widget specific data persistently

enum WidgetRender
{
    WidgetRender_None = 0,   // It doesn't have any data associated to draw
    WidgetRender_Translate,  // Translate gizmo
    WidgetRender_Rotate,     // Rotate gizmo
    WidgetRender_Scale       // Scale gizmo
};

struct Widget
{
    static u64 frameCounter;
    u64 lastUsedFrame;
    
    // Any state that might be used by any widget
    
    // Quaternion euler angle drag UI
    Vec3 eulerAngles;
    
    // 3D gizmos
    bool clickedX;
    bool clickedY;
    bool clickedZ;
    bool clickedXY;
    bool clickedXZ;
    bool clickedYZ;
    Vec3 vecDragStart;
    Vec3 dragStartMousePos;
    Quat quatDragStart;
    
    // For rendering (at the end of the frame)
    WidgetRender renderMode;
    Vec3 widgetPos;
    Quat widgetRot;
    Vec3 widgetScale;  // Used for scale gizmo, when pulling on one or more axes
};

void RemoveUnusedWidgets(Editor* editor);
// Returns whether it was present or not
bool AddWidgetIfNotPresent(Editor* editor, ImGuiID id, Widget initialState);

// Immediate mode gizmos

// These gizmos are actually rendered last, so we just save all the state
bool TranslationGizmo(const char* id, Editor* editor, Vec3* pos);
bool AxisHandle(Ray cameraRay, Vec3* pos, Vec3 dir, Vec4 color, float scale, bool* clicked, Vec3* dragStart, Vec3* dragStartMousePos);
Vec3 ProjectToLine(Ray cameraRay, Vec3 pos, Vec3 dir, bool* outSuccess);
bool RotationGizmo(Quat* rot);
bool ScaleGizmo(Vec3* scale);

float GetScreenspaceToWorldDistance(Editor* editor, Vec3 pos);
// TODO: @tmp Quick function that we use for now. Will later be
// pushed to the renderer
void DrawQuickLine(Vec3 v1, Vec3 v2, float scale, Vec4 color);