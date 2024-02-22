
#include "base.h"
#include "os/os_generic.h"
#include "simulation.h"
#include "renderer/renderer_opengl.h"
#include "renderer/renderer_d3d11.h"

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
        case GfxLib_None:   return 1;
        case GfxLib_OpenGL: glRenderer = gl_InitRenderer(&permArena);       break;
        case GfxLib_D3D11:  d3d11Renderer = d3d11_InitRenderer(&permArena); break;
    }
    
    AppState appState = InitSimulation();
    
    OS_ShowWindow();
    
    const float maxDeltaTime = 1/20.0f;
    float deltaTime = 1/60.0f;  // Reasonable default value
    uint64_t startTicks = 0;
    uint64_t endTicks = 0;
    
    bool firstIter = true;
    while(true)
    {
        bool quit = OS_HandleWindowEvents();
        if(quit) break;
        
        InputState input = OS_PollInput();
        
        if(!firstIter)
        {
            endTicks = OS_GetTicks();
            deltaTime = min(maxDeltaTime, OS_GetElapsedSeconds(startTicks, endTicks));
        }
        
        startTicks = OS_GetTicks();
        MainUpdate(&appState, deltaTime, input, &permArena, &frameArena);
        
        if(!firstIter) OS_SwapBuffers();
        
        switch(usedLib)
        {
            case GfxLib_None:   return 1;
            case GfxLib_OpenGL: gl_Render(&glRenderer, appState.renderSettings);       break;
            case GfxLib_D3D11:  d3d11_Render(&d3d11Renderer, appState.renderSettings); break;
        }
        
        firstIter = false;
    }
    
    return 0;
}
