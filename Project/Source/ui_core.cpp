
#include "ui_core.h"

UI_Key UI_KeyNull()
{
    return 0;
}

UI_Key UI_KeyFromString(String string)
{
    return Murmur64(string.ptr, string.len);
}

UI_Box* UI_MakeBox(UI_BoxFlags flags, String string)
{
    // Using malloc instead of arena for now...
    auto box = (UI_Box*)malloc(sizeof(UI_Box));
    memset(box, 0, sizeof(UI_Box));
    
    box->key = UI_KeyFromString(string);
    box->lastFrameTouchedIdx = 0; // TODO ???
    box->flags  = flags;
    box->string = string;
    return box;
}

UI_Box* UI_MakeBox(UI_BoxFlags flags, const char* fmt, ...)
{
    // Process the format string...
    
    String resStr = {0};
    
    // For this i would imagine that you would use last frame's data?
    UI_MakeBox(flags, resStr);
    
    return nullptr;
}

UI_Box* UI_MakeBox(UI_BoxFlags flags, const char* fmt, va_list args)
{
    TODO;
    return nullptr;
}

void UI_BoxEquipDisplayString(UI_Box* box, String string)
{
    box->string = string;
    box->key    = UI_KeyFromString(string);
}

void UI_BoxEquipChildLayoutAxis(UI_Box* box, Axis2 axis)
{
    TODO;
}

UI_Box* UI_PushParent(UI_Box* box)
{
    // Just add this to the parent stack
    // (and i guess just return the arg?)
    TODO;
    
    return nullptr;
}

UI_Box* UI_PopParent()
{
    assert(ui.parentLast && "Error: Trying to pop a parent with an empty parent stack!");
    
    ui.parentLast = ui.parentLast->prev;
    ui.parentLast->next = nullptr;
    return ui.parentLast->box;
}

UI_Signal UI_SignalFromBox(UI_Box* box)
{
    TODO;
    return {0};
}

UI_Signal UI_Button(String string)
{
    UI_Box* box = UI_MakeBox(UI_BoxFlag_Clickable  |
                             UI_BoxFlag_DrawBorder |
                             UI_BoxFlag_DrawText   |
                             UI_BoxFlag_DrawBackground |
                             UI_BoxFlag_HotAnimation |
                             UI_BoxFlag_ActiveAnimation,
                             string);
    return UI_SignalFromBox(box);
}

void UI_AutoLayout(UI_Box* root)
{
    ScratchArena scratch;
    
    for(int i = 0; i < Axis2_Count; ++i)
    {
        Axis2 axis = (Axis2)i;
        UI_ComputeStandaloneSize(root, axis);
        UI_ComputeParentDependentSize(root, axis);
        UI_ComputeChildDependentSize(root, axis);
        UI_SolveSizeViolations(root, axis);
        UI_ComputePositions(root, axis);
    }
}

void UI_ComputeStandaloneSize(UI_Box* box, Axis2 axis)
{
    if(!box) return;
    
    // @temp
    auto kind = box->semanticSize[axis].kind;
    switch(kind)
    {
        default: break;
        case UI_SizeKind_Null:        box->computedSize[axis] = 0.0f;  break;
        case UI_SizeKind_Pixels:      box->computedSize[axis] = box->semanticSize[axis].value; break;
        case UI_SizeKind_TextContent: break;
    }
    
    // This could be post-order, doesn't matter
    UI_ComputeStandaloneSize(box->first, axis);
    UI_ComputeStandaloneSize(box->next, axis);
}

void UI_ComputeParentDependentSize(UI_Box* box, Axis2 axis)
{
    if(!box) return;
    
    auto kind = box->semanticSize[axis].kind;
    if(kind == UI_SizeKind_PercentOfParent)
    {
        if(box->parent)
            box->computedSize[axis] = box->parent->computedSize[axis];
    }
    
    UI_ComputeParentDependentSize(box->first, axis);
    UI_ComputeParentDependentSize(box->next, axis);
}

void UI_ComputeChildDependentSize(UI_Box* box, Axis2 axis)
{
    if(!box) return;
    
    UI_ComputeChildDependentSize(box->first, axis);
    UI_ComputeChildDependentSize(box->next, axis);
    
    auto kind = box->semanticSize[axis].kind;
    if(kind == UI_SizeKind_ChildrenSum)
    {
        float sumSize = 0.0f;
        for(UI_Box* curBox = box->first; curBox; curBox = curBox->next)
            sumSize += curBox->computedSize[axis];
        
        box->computedSize[axis] = sumSize;
    }
}

void UI_SolveSizeViolations(UI_Box* box, Axis2 axis)
{
    if(!box) return;
    
    auto kind = box->semanticSize[axis].kind;
    TODO;
    
    UI_SolveSizeViolations(box->first, axis);
    UI_SolveSizeViolations(box->next, axis);
}

void UI_ComputePositions(UI_Box* box, Axis2 axis)
{
    if(!box) return;
    
    TODO;
    
    UI_ComputePositions(box->first, axis);
    UI_ComputePositions(box->next, axis);
}
