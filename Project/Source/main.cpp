
#include "base.h"
#include "os/os_generic.h"
#include "core.h"
#include "editor.h"
#include "renderer/renderer_generic.h"
#include "sound/sound_generic.h"

void SetWorkingDirRelativeToExe(const char* path);

int main()
{
    InitScratchArenas();
    InitPermArena();
    
    // TODO: @tmp StringMap testing
    StringMap<int> map = {};
    Append(&map, "hello", 3);
    Append(&map, "world", 5);
    
    {
        char staticString[10] = "hello";
        for(int i = 0; i < 20; ++i)
        {
            staticString[5] = 'a' + i;
            staticString[6] = '\0';
            Append(&map, staticString, i);
        }
    }
    
    {
        char staticString[10] = {'w', 'o', 'r', 'l', 'd', '\0'};
        for(int i = 0; i < 20; ++i)
        {
            staticString[6] = 'a' + i;
            staticString[7] = '\0';
            Append(&map, staticString, i);
        }
    }
    
    {
        char staticString[10] = {'h', 'e', '\0'};
        for(int i = 0; i < 20; ++i)
        {
            staticString[2] = 'a' + i;
            staticString[3] = '\0';
            Append(&map, staticString, i);
        }
    }
    
    {
        char staticString[10] = {'h', 'e', 'l', '\0'};
        for(int i = 0; i < 20; ++i)
        {
            staticString[3] = 'a' + i;
            staticString[4] = '\0';
            Append(&map, staticString, i);
        }
    }
    
    auto res1 = Lookup(&map, "hello");
    auto res2 = Lookup(&map, "world");
    Log("%d %d", res1.res, res2.res);
    
    Free(&map);
    
    OS_Init("Simple Game Engine");
    defer { OS_Cleanup(); };
    
    SetWorkingDirRelativeToExe("../../Assets/");
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    defer
    {
        OS_DearImguiShutdown();
        R_ShutdownDearImgui();
        ImGui::DestroyContext();
    };
    
    Arena frameArena = ArenaVirtualMemInit(GB(4), MB(2));
    
    R_Init();
    defer { R_Cleanup(); };
    
    S_Init();
    defer { S_Cleanup(); };
    
    InitAssetSystem();
    
    EntityManager entManager = InitEntityManager();
    defer { FreeEntities(&entManager); };
    
    OS_InitDearImgui();
    Editor editor = InitEditor(&entManager);
    
    OS_ShowWindow();
    
    const float maxDeltaTime = 1/20.0f;
    float deltaTime = 0.0f;
    u64 startTicks  = 0;
    u64 endTicks    = 0;
    
    bool firstIter = true;
    bool frameInFlight = false;
    while(bool proceed = OS_HandleWindowEvents())
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
        
        // If the previous frame hasn't been fully rendered at this point,
        // then we stall the CPU until it is.
        if(frameInFlight) OS_SwapBuffers();
        
        MainRender(&entManager, &editor, deltaTime, &frameArena);
        
        ArenaFreeAll(&frameArena);
        firstIter = false;
        
        if(OS_NeedThisFrameBeforeNextIteration())
        {
            OS_SwapBuffers();
            frameInFlight = false;
        }
        else
            frameInFlight = true;
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