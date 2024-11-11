
#include "base.h"
#include "os/os_generic.h"
#include "core.h"
#include "editor.h"
#include "renderer_backend/generic.h"
#include "sound/sound_generic.h"
#include "renderer_frontend.h"

void SetWorkingDirRelativeToExe(const char* path);

int main()
{
    InitScratchArenas();
    InitPermArena();
    
    OS_Init("Simple Game Engine");
    defer { OS_Cleanup(); };
    
    SetWorkingDirRelativeToExe("../../Assets/");
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    defer
    {
        OS_DearImguiShutdown();
        R_ImGuiShutdown();
        ImGui::DestroyContext();
    };
    
    Arena frameArena = ArenaVirtualMemInit(GB(4), MB(2));
    
    R_Init();
    defer { R_Cleanup(); };
    
    //AssetSystemInit();
    //defer { AssetSystemCleanup(); };
    
    RenderResourcesInit();
    defer { RenderResourcesCleanup(); };
    
#ifdef Development
    OS_StartFileWatcher(".");  // Watch files in Assets directory (and subfolders)
    defer { OS_StopFileWatcher(); };
#endif
    
    //InitAssetSystem();
    
    EntityManager entManager = InitEntityManager();
    defer { FreeEntities(&entManager); };
    
    OS_DearImguiInit();
    R_ImGuiInit();
    
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
        
        s32 w, h;
        OS_GetClientAreaSize(&w, &h);
        
        R_UpdateSwapchainSize();
        R_SetViewport(0, 0, w, h);
        
        RenderFrame(&entManager);
        
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