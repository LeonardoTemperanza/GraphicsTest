
#include "base.h"
#include "os/os_generic.h"
#include "core.h"
#include "editor.h"
#include "renderer_backend/generic.h"
#include "sound/sound_generic.h"

void SetWorkingDirRelativeToExe(const char* path);

int main()
{
    InitScratchArenas();
    InitPermArena();
    
    //printf("%d");
    
    OS_Init("Simple Game Engine");
    defer { OS_Cleanup(); };
    
    SetWorkingDirRelativeToExe("../../Assets/");
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    defer
    {
        OS_DearImguiShutdown();
        //R_DearImguiShutdown();
        ImGui::DestroyContext();
    };
    
    Arena frameArena = ArenaVirtualMemInit(GB(4), MB(2));
    
    R_Init();
    //R_SetToDefaultState();
    defer { R_Cleanup(); };
    
    S_Init();
    defer { S_Cleanup(); };
    
#ifdef Development
    OS_StartFileWatcher(".");  // Watch files in Assets directory (and subfolders)
    defer { OS_StopFileWatcher(); };
#endif
    
    //InitAssetSystem();
    
    EntityManager entManager = InitEntityManager();
    defer { FreeEntities(&entManager); };
    
    OS_DearImguiInit();
    //R_DearImguiInit();
    
    Editor editor = InitEditor(&entManager);
    
    OS_ShowWindow();
    
    const float maxDeltaTime = 1/20.0f;
    float deltaTime = 0.0f;
    u64 startTicks  = 0;
    u64 endTicks    = 0;
    
    bool firstIter = true;
    while(true)
    {
        // In the first iteration, we simply want to render
        // the initialized state.
        if(!firstIter)
        {
            endTicks = OS_GetTicks();
            deltaTime = min(maxDeltaTime, (float)OS_GetElapsedSeconds(startTicks, endTicks));
            startTicks = OS_GetTicks();
        }
        
        MainUpdate(&entManager, &editor, deltaTime, &frameArena);
        
        R_WaitLastFrame();
        
        // Handle window events only now, because we only want to allow
        // resizing after we've finished the last frame.
        bool proceed = OS_HandleWindowEvents();
        if(!proceed) break;
        
        //MainRender(&entManager, &editor, deltaTime, &frameArena);
        const R_Framebuffer* screen = R_GetScreen();
        R_FramebufferClear(screen, BufferMask_Depth & BufferMask_Stencil);
        R_FramebufferFillColorFloat(screen, 0, 0.5f, 0.5f, 0.5f, 1.0f);
        R_PresentFrame();
        
        ArenaFreeAll(&frameArena);
        firstIter = false;
    }
    
    return 0;
}

void SetWorkingDirRelativeToExe(const char* path)
{
    StringBuilder assetsPath = {0};
    defer { FreeBuffers(&assetsPath); };
    
    char* exePath = GetExecutablePath();
    defer { free(exePath); };
    
    // Get rid of the .exe file itself in the path
    int len = (int)strlen(exePath);
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
    Append(&assetsPath, path);
    NullTerminate(&assetsPath);
    B_SetCurrentDirectory(ToString(&assetsPath).ptr);
}