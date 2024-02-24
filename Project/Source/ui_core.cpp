
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
    
    return UI_MakeBox(flags, resStr);
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
    TODO;
    return nullptr;
}

UI_Box* UI_PopParent()
{
    TODO;
    return nullptr;
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