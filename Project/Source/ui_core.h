
#pragma once

#include "base.h"

enum UI_SizeKind
{
    UI_SizeKind_Null = 0,
    UI_SizeKind_Pixels,
    UI_SizeKind_TextContent,
    UI_SizeKind_PercentOfParent,
    UI_SizeKind_ChildrenSum,
};

struct UI_Size
{
    UI_SizeKind kind;
    f32 value;
    f32 strictness;
};

enum Axis2
{
    Axis2_X,
    Axis2_Y,
    Axis2_Count,
};

struct UI_Key
{
    int key;
};

typedef u32 UI_BoxFlags;
enum
{
    UI_BoxFlag_Clickable       = 1<<0,
    UI_BoxFlag_ViewScroll      = 1<<1,
    UI_BoxFlag_DrawText        = 1<<2,
    UI_BoxFlag_DrawBorder      = 1<<3,
    UI_BoxFlag_DrawBackground  = 1<<4,
    UI_BoxFlag_DrawDropShadow  = 1<<5,
    UI_BoxFlag_Clip            = 1<<6,
    UI_BoxFlag_HotAnimation    = 1<<7,
    UI_BoxFlag_ActiveAnimation = 1<<8,
};

// The basic building box to construct
// widgets; from the simpler ones (buttons)
// to the more complex ones (List boxes, combo boxes, etc.)
struct UI_Box
{
    // Box hierarchy
    UI_Box* first;
    UI_Box* last;
    UI_Box* next;
    UI_Box* prev;
    UI_Box* parent;
    
    // Hash table 
    UI_Box* hashNext;
    UI_Box* hashPrev;
    
    // Key+generation info
    UI_Key key;
    u64 lastFrameTouchedIdx;
    
    // Per frame info provided by builders
    UI_BoxFlags flags;
    String string;
    UI_Size semanticSize[Axis2_Count];
    
    // Recomputed every frame
    float computedRelPosition[Axis2_Count];
    float computedSize[Axis2_Count];
    Vec2 rectMin;
    Vec2 rectMax;
    
    // Persistent data
    f32 hotT;   // Used for animation
    f32 timeT;  // Used for animation
};

// Bacic key type helpers
UI_Key UI_KeyNull();
UI_Key UI_KeyFromString(String string);
b32 UI_KeyMatch(UI_Key a, UI_Key b);

// Construct a widget, looking up from the cache if
// possible, and pushing it as a new child of the
// active parent
UI_Box* UI_MakeBox(UI_BoxFlags flags, String string);
UI_Box* UI_MakeBox(UI_BoxFlags flags, char* fmt, ...);

// Some other possible building parameterizations
void UI_BoxEquipDisplayString(UI_Box* widget, String string);
void UI_BoxEquipChildLayoutAxis(UI_Box* widget, Axis2 axis);

// Managing the parent stack
UI_Box* UI_PushParent(UI_Box* widget);
UI_Box* UI_PopParent();

// The user's interaction with a widget
struct UI_Signal
{
    UI_Box* widget;
    Vec2 mouse;
    Vec2 dragDelta;
    bool clicked;
    bool doubleClicked;
    bool rightClicked;
    bool pressed;
    bool released;
    bool dragging;
    bool hovering;
};

UI_Signal UI_SignalFromBox(UI_Box* widget);

// Helpers from common widget "types", though the concept
// of subtypes does not really exist here, there's just a single
// Box struct that just has everything you might ever need.

UI_Signal UI_Button(String string);
