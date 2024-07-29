
#pragma once

// Editor stuff

#include "core.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"

struct QueryElement
{
    EntityFlags flag;
    bool enabled;
};

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
    bool inEditor;
    
    bool focusOnConsole;
    
    float consoleAnimation;  // 0 when fully hidden, 1 when fully visible
    
    Array<Entity*> selected;
    
    // Rendering
    R_Pipeline outlineForIntTexture;
    
    // Entity list query
    Array<QueryElement> queryElements;
    EntityFlags entityQuery;
    
    // Editor camera
    float horizontalFOV;
    float nearClip;
    float farClip;
    Vec3 camPos;
    Quat camRot;
};

// TODO: @tmp This is mostly taken from the example
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
void ShowMainMenuBar(Editor* editor);
void ShowEntityList(Editor* editor);
void ShowEntityAndChildren(Editor* editor, Entity* entity, Slice<Slice<Entity*>> childrenPerEntity);
void UpdateEditorCamera(Editor* editor, float deltaTime);

int GetEntitySelectionId(Editor* ui, Entity* entity);
void SelectEntity(Editor* ui, Entity* entity);

// Immediate mode gizmos
void TranslationGizmo(const char* id, Editor* editor, Vec3* pos);
void RotationGizmo(Quat* rot);
void ScaleGizmo(Vec3* scale);

// Console print utilities
void ClearLog();
void Log(const char* fmt, ...);
void ExecuteCommand(const char* command);
int TextEditCallback(ImGuiInputTextCallbackData* data);

void ShowConsole(Editor* ui);

// From: https://github.com/ocornut/imgui/issues/1831 by volcoma
static void PushMultiItemsWidthsAndLabels(const char* labels[], int components, float w_full);
bool DragFloatNEx(const char* labels[], float* v, int components, float v_speed, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f");