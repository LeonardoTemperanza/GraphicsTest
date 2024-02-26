
#pragma once

#include "base.h"

// Ok, so i guess we would need to have a pool of nodes right?
// And every frame we just query the existing data structure,
// and if a certain node is "new" we allocate a new one, correct?

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
    Axis2_X = 0,
    Axis2_Y,
    Axis2_Count,
};

typedef s64 UI_Key;

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
    
    // Key info
    UI_Key key;
    
    // Per-frame info provided by builders
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
    f32 activeT;  // Used for animation
};

struct UI_ParentLL
{
    UI_Box* box;
    UI_ParentLL* next;
    UI_ParentLL* prev;
};

// 503 because it's a prime number
#define UI_HashSize 503
struct UI_Ctx
{
    InputState input;
    
    // Double buffer the arena for cross frame
    // persistent data.
    Arena prevArena;
    Arena arena;
    
    // Used to manage the parent stack. By "parent" i mean
    // the currently selected parent by the UI builder code.
    UI_ParentLL* parentFirst;
    UI_ParentLL* parentLast;
    
    // Hash containing the nodes, for cross-frame data.
    // Entries can be nullptr, in which case they are not occupied.
    Slice<UI_Box*> prevHash;
    Slice<UI_Box*> hash;
};

static UI_Ctx ui;

// Bacic key type helpers
UI_Key UI_KeyNull();
UI_Key UI_KeyFromString(String string);
b32 UI_KeyMatch(UI_Key a, UI_Key b);

// Construct a box, looking up from the cache if
// possible, and pushing it as a new child of the
// active parent
UI_Box* UI_MakeBox(UI_BoxFlags flags, String string);
UI_Box* UI_MakeBox(UI_BoxFlags flags, const char* fmt, ...);

// Some other possible building parameterizations
void UI_BoxEquipDisplayString(UI_Box* box, String string);
void UI_BoxEquipChildLayoutAxis(UI_Box* box, Axis2 axis);

// Managing the parent stack
UI_Box* UI_PushParent(UI_Box* box);
UI_Box* UI_PopParent();

// The user's interaction with a box
struct UI_Signal
{
    UI_Box* box;
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

// @temp I assume what this does is this:
// it takes the current box as argument
// it attempts to get the previous frame's
// equivalent by getting the key, and looking
// that up the cache (to get its final layout).
// then you compare that with the current frame's
// input and then 
UI_Signal UI_SignalFromBox(UI_Box* box);

// Helpers from common widget "types", though the concept
// of subtypes does not really exist here, because the needed
// features are obtained by composing simple elements (boxes)
// together.

UI_Box* UI_Container();
UI_Signal UI_Button(char* string);
float UI_Slider(float curValue, float min, float max, const char* text);
b32 UI_Checkbox(b32 curValue, const char* text);

// Common API
void UI_Init();
void UI_BeginFrame(InputState input);
void UI_EndFrame();

// Layout
void UI_AutoLayout(UI_Box* root);
void UI_ComputeStandaloneSize(UI_Box* box, Axis2 axis);
void UI_ComputeParentDependentSize(UI_Box* box, Axis2 axis);
void UI_ComputeChildDependentSize(UI_Box* box, Axis2 axis);
void UI_SolveSizeViolations(UI_Box* box, Axis2 axis);
void UI_ComputePositions(UI_Box* box, Axis2 axis);
