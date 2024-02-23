
#include "ui_core.h"

#if 0
UI_Signal UI_Button(String string)
{
    UI_Box* widget = UI_BoxMake(UI_BoxFlag_Clickable  |
                                UI_BoxFlag_DrawBorder |
                                UI_BoxFlag_DrawText   |
                                UI_BoxFlag_DrawBackground |
                                UI_BoxFlag_HotAnimation |
                                UI_BoxFlag_ActiveAnimation,
                                string);
    return UI_SignalFromBox(widget);
}
#endif