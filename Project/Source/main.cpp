
#include "base.h"
#include "os/os_generic.h"
#include "simulation.h"
#include "renderer/renderer_generic.h"
#include "ui_core.h"

void SetWorkingDirToAssets()
{
    StringBuilder assetsPath = {0};
    defer { FreeBuffers(&assetsPath); };
    
    char* exePath = OS_GetExecutablePath();
    defer { free(exePath); };
    
    // Get rid of the .exe file itself in the path
    int len = strlen(exePath);
    int lastSeparator = len - 1;
    for(int i = len-1; i >= 0; --i)
    {
        if(exePath[i] == '/' || exePath[i] == '\\')
        {
            lastSeparator = i;
            break;
        }
    }
    
    String exePathNoFile = {.ptr=exePath, .len=lastSeparator+1};
    Append(&assetsPath, exePathNoFile);
    Append(&assetsPath, "../../Assets/");
    NullTerminate(&assetsPath);
    OS_SetCurrentDirectory(ToString(&assetsPath).ptr);
}

int main()
{
    OS_GraphicsLib usedLib = OS_Init();
    defer { OS_Cleanup(); };
    
    SetWorkingDirToAssets();
    
    char* curDir = OS_GetCurrentDirectory();
    
    Arena permArena = ArenaVirtualMemInit(GB(4), MB(2));
    Arena frameArena = ArenaVirtualMemInit(GB(4), MB(2));
    
    SetRenderFunctionPointers(usedLib);
    Renderer* renderer = InitRenderer(&permArena);
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
        
        Render(renderer, appState.renderSettings);
        
        firstIter = false;
    }
    
    return 0;
}
