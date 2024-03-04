
#include "ui_core.h"

UI_Key UI_KeyNull()
{
    return 0;
}

UI_Key UI_KeyFromString(String string)
{
    return Murmur64(string.ptr, string.len);
}

b32 UI_KeyMatch(UI_Key a, UI_Key b)
{
    return a == b;
}

UI_Box* UI_MakeBox(UI_BoxFlags flags, String string)
{
    assert(ui.parentLast);
    
    // Using malloc instead of arena for now...
    auto box = ArenaAllocType(UI_Box, &ui.arena);
    memset(box, 0, sizeof(UI_Box));
    
    box->key = UI_KeyFromString(string);
    box->flags  = flags;
    box->string = string;
    
    // Set as child of currently selected parent
    if(ui.parentLast)
    {
        assert(ui.parentLast->box);
        if(!ui.parentLast->box->last)
            ui.parentLast->box->first = ui.parentLast->box->last = box;
        else
        {
            ui.parentLast->box->last->next = box;
            box->prev = ui.parentLast->box->last->next;
            ui.parentLast->box->last = box;
        }
    }
    
    // Look for persistent data in the previous frame's hash
    for(UI_Box* cur = ui.prevHash[box->key]; cur; cur->hashNext)
    {
        if(cur->string == box->string)  // Found previous frame's box
        {
            // Set persistent data
            box->hotT = cur->hotT;
            box->activeT = cur->activeT;
            break;
        }
    }
    
    return box;
}

UI_Box* UI_MakeBox(UI_BoxFlags flags, const char* fmt, ...)
{
    // Process the format string...
    TODO;
    
    String resStr = {0};
    
    // For this i would imagine that you would use last frame's data?
    UI_MakeBox(flags, resStr);
    
    return nullptr;
}

UI_Box* UI_MakeBox(UI_BoxFlags flags, const char* fmt, va_list args)
{
    // Make the string and then just call the usual function
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
    box->semanticSize[axis].kind = UI_SizeKind_ChildrenSum;
}

UI_Box* UI_PushParent(UI_Box* box)
{
    auto node = ArenaAllocType(UI_ParentLL, &ui.arena);
    memset(node, sizeof(*node), 0);
    
    if(!ui.parentLast)
    {
        node->box = box;
        node->next = nullptr;
        node->prev = nullptr;
        
        ui.parentFirst = ui.parentLast = node;
    }
    else
    {
        node->prev = ui.parentLast;
        ui.parentLast->next = node;
        ui.parentLast = node;
    }
    
    return box;
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
    // Look for a box with the same key in the hash
    UI_Box* found = nullptr;
    for(UI_Box* cur = ui.prevHash[box->key]; cur; cur = cur->hashNext)
    {
        if(cur->string == box->string)
        {
            found = cur;
            break;
        }
    }
    
    if(!found) return {0};
    
    // Check for input on the given box
    UI_Signal signal = {0};
    ui.input.mouse.xPos;
    TODO;
    
    return signal;
}

UI_Signal UI_Button(char* string)
{
    UI_Box* box = UI_MakeBox(UI_BoxFlag_Clickable  |
                             UI_BoxFlag_DrawBorder |
                             UI_BoxFlag_DrawText   |
                             UI_BoxFlag_DrawBackground |
                             UI_BoxFlag_HotAnimation |
                             UI_BoxFlag_ActiveAnimation,
                             ToLenStr(string));
    return UI_SignalFromBox(box);
}

void UI_Init()
{
    ui = {0};
    ui.arena = ArenaVirtualMemInit(GB(4), MB(2));
    ui.prevArena = ArenaVirtualMemInit(GB(4), MB(2));
}

void UI_BeginFrame(OS_InputState input)
{
    ui.input = input;
}

void UI_EndFrame()
{
    ui.parentLast = ui.parentFirst = nullptr;
    
    ArenaFreeAll(&ui.prevArena);
    auto tmp = ui.prevArena;
    ui.prevArena = tmp;
    ui.arena = tmp;
    
    auto hash = ui.hash;
    ui.prevHash = hash;
    ui.hash.ptr = ArenaAllocArray(UI_Box*, UI_HashSize, &ui.arena);
    ui.hash.len = UI_HashSize;
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
    
    auto kind = box->semanticSize[axis].kind;
    switch(kind)
    {
        default: break;
        case UI_SizeKind_Null:        box->computedSize[axis] = 0.0f;  break;
        case UI_SizeKind_Pixels:      box->computedSize[axis] = box->semanticSize[axis].value; break;
        case UI_SizeKind_TextContent: break; // @temp
    }
    
    // This could be post-order, doesn't matter
    UI_ComputeStandaloneSize(box->first, axis);
    UI_ComputeStandaloneSize(box->next, axis);
}

void UI_ComputeParentDependentSize(UI_Box* box, Axis2 axis)
{
    if(!box) return;
    
    auto semSize = box->semanticSize[axis];
    if(semSize.kind == UI_SizeKind_PercentOfParent)
    {
        float percent = semSize.value;
        if(box->parent)
            box->computedSize[axis] = box->parent->computedSize[axis] * percent;
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
        for(UI_Box* child = box->first; child; child = child->next)
            sumSize += child->computedSize[axis];
        
        box->computedSize[axis] = sumSize;
    }
}

void UI_SolveSizeViolations(UI_Box* box, Axis2 axis)
{
    if(!box) return;
    
    auto kind = box->semanticSize[axis].kind;
    float sumSize = 0.0f;
    int numChildren = 0;
    for(UI_Box* child = box->first; child; child = child->next)
    {
        sumSize += child->computedSize[axis];
        ++numChildren;
    }
    
    float violationSize = sumSize - box->computedSize[axis];
    for(UI_Box* child = box->first; child; child = child->next)
    {
        // Adjust the child's size, as much as its strictness allows
        auto& childSize = child->computedSize[axis];
        float strictness = child->semanticSize[axis].strictness;
        float prevSize = childSize;
        childSize -= violationSize / numChildren;
        childSize = max(prevSize * strictness, childSize);
    }
    
    UI_SolveSizeViolations(box->first, axis);
    UI_SolveSizeViolations(box->next, axis);
}

void UI_ComputePositions(UI_Box* box, Axis2 axis)
{
    if(!box) return;
    
    // How to do this? Would i not need the direction of travel of the nodes?
    // I guess i can just add it to the box struct, no?
    // One thing that i would need to do is actually just play around with dear imgui
    TODO;
    
    UI_ComputePositions(box->first, axis);
    UI_ComputePositions(box->next, axis);
}
