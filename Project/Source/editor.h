
#pragma once

// Editor stuff

#include "entities.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "generated/introspection.h"
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
    EntityManager* man;
    
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
    
    Array<EntityKey> selected;
    
    // Rendering
    R_Framebuffer selectedFramebuffer;  // Serves to draw selected objects to
    R_Framebuffer entityIdFramebuffer;  // Serves to draw entity id to (for clicking on entities)
    
    // Entity list query
    Array<QueryElement> queryElements;
    EntityFlags entityQuery;
    
    // Editor camera
    Vec3 camPos;
    Quat camRot;
    
    struct CameraParams
    {
        float fov;
        float nearClip;
        float farClip;
    };
    
    CameraParams camParams;
    
    // Gizmo and custom UI state
    
    HashMap<ImGuiID, Widget> widgetTable;
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

Editor InitEditor(EntityManager* man);
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
// We're also calling printf every time we log.
// this is in part to enable the compiler warnings
// we get when getting the format specifier wrong.
#define Log(fmt, ...) do { EditorLog(fmt, __VA_ARGS__); printf(fmt "\n", __VA_ARGS__); } while(0)
void EditorLog(const char* fmt, ...);
void ExecuteCommand(const char* command);
int TextEditCallback(ImGuiInputTextCallbackData* data);

void ShowConsole(Editor* ui);

// Introspection
void ShowEntityControl(Editor* editor, Entity* entity);
void ShowStructControl(MetaStruct metaStruct, Editor* editor, void* address);

// Dear Imgui helpers for basic types
void ShowVec3Control(const char* strId, Vec3* value, float columnWidth, float sensitivity);
void ShowQuatControl(const char* strId, Editor* editor, Quat* value, float columnWidth, float sensitivity);

// NOTE: Sometimes we want to store some permanent
// data that is specifically tied to a particular
// instance of a widget. ImGui doesn't let us attach
// arbitrary data to any widget (rightly so), so instead
// we have our own hash data structure which is parallel
// to imgui's. This is why we have a hash table of this Widget
// structure.

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
    
    // Text inputs
    char text[512];
    
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

void RemoveUnusedWidgets(HashMap<ImGuiID, Widget>* table);
// Returns the state that was already in the table or the newly added one
// newly added ones are zero initialized
struct GetOrAddWidgetReturn
{
    Widget* widget;
    bool found;
};
GetOrAddWidgetReturn GetOrAddWidget(HashMap<ImGuiID, Widget>* table, ImGuiID id);

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
