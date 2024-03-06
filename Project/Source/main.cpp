
#include "base.h"
#include "os/os_generic.h"
#include "simulation.h"
#include "renderer/renderer_generic.h"
#include "ui_core.h"

int main()
{
    OS_GraphicsLib usedLib = OS_Init();
    defer { OS_Cleanup(); };
    
    // TODO: Get the exe directory so it doesn't
    // depend on the current working directory
    OS_SetCurrentDirectory("../../Assets/");
    
    Arena permArena = ArenaVirtualMemInit(GB(4), MB(2));
    Arena frameArena = ArenaVirtualMemInit(GB(4), MB(2));
    
    SetRenderFunctionPointers(usedLib);
    Renderer renderer = InitRenderer(&permArena);
    AppState appState = InitSimulation();
    UI_Init();
    
    OS_ShowWindow();
    
    const float maxDeltaTime = 1/20.0f;
    float deltaTime = 0.0f;
    u64 startTicks  = 0;
    u64 endTicks    = 0;
    
    bool firstIter = true;
    while(bool proceed = OS_HandleWindowEvents())
    {
        // In the first iteration, we simply want to render
        // the initialized state.
        if(!firstIter)
        {
            endTicks = OS_GetTicks();
            deltaTime = min(maxDeltaTime, OS_GetElapsedSeconds(startTicks, endTicks));
            
            startTicks = OS_GetTicks();
            MainUpdate(&appState, deltaTime, &permArena, &frameArena);
            OS_SwapBuffers();
        }
        
        Render(&renderer, appState.renderSettings);
        
        firstIter = false;
    }
    
    return 0;
}
