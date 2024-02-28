
#include "base.h"
#include "os/os_generic.h"
#include "simulation.h"
#include "renderer/renderer_opengl.h"
#include "renderer/renderer_d3d11.h"
#include "ui_core.h"

int main()
{
    OS_GraphicsLib usedLib = OS_Init();
    defer { OS_Cleanup(); };
    
    Arena permArena = ArenaVirtualMemInit(GB(4), MB(2));
    Arena frameArena = ArenaVirtualMemInit(GB(4), MB(2));
    
    gl_Renderer glRenderer = {0};
    d3d11_Renderer d3d11Renderer = {0};
    switch(usedLib)
    {
        case GfxLib_None:   break;
        case GfxLib_OpenGL: glRenderer = gl_InitRenderer(&permArena);       break;
        case GfxLib_D3D11:  d3d11Renderer = d3d11_InitRenderer(&permArena); break;
    }
    
    AppState appState = InitSimulation();
    UI_Init();
    
    OS_ShowWindow();
    
    const float maxDeltaTime = 1/20.0f;
    float deltaTime = 0.0f;
    u64 startTicks  = 0;
    u64 endTicks    = 0;
    
    bool firstIter = true;
    while(true)
    {
        bool quit = OS_HandleWindowEvents();
        if(quit) break;
        
        InputState input = OS_PollInput();
        
        // In the first iteration, we simply want to render
        // the initialized state.
        if(!firstIter)
        {
            endTicks = OS_GetTicks();
            deltaTime = min(maxDeltaTime, OS_GetElapsedSeconds(startTicks, endTicks));
            
            startTicks = OS_GetTicks();
            MainUpdate(&appState, deltaTime, input, &permArena, &frameArena);
            OS_SwapBuffers();
        }
        
        switch(usedLib)
        {
            case GfxLib_None:   break;
            case GfxLib_OpenGL: gl_Render(&glRenderer, appState.renderSettings);       break;
            case GfxLib_D3D11:  d3d11_Render(&d3d11Renderer, appState.renderSettings); break;
        }
        
        firstIter = false;
    }
    
    return 0;
}
