
#include "base.h"
#include "os/os_generic.h"
#include "core.h"
#include "renderer/renderer_generic.h"

void SetWorkingDirToAssets();

int main()
{
    OS_Init("Simple Game Engine");
    defer { OS_Cleanup(); };
    
    SetWorkingDirToAssets();
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    defer
    {
        OS_DearImguiShutdown();
        R_ShutdownDearImgui();
        ImGui::DestroyContext();
    };
    
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();
    
    OS_InitDearImgui();
    
    Arena permArena = ArenaVirtualMemInit(GB(4), MB(2));
    Arena frameArena = ArenaVirtualMemInit(GB(4), MB(2));
    
    R_Init();
    defer { R_Cleanup(); };
    
    Entities* entities = InitEntities();
    defer { FreeEntities(entities); };
    
    OS_ShowWindow();
    
    const float maxDeltaTime = 1/20.0f;
    float deltaTime = 0.0f;
    u64 startTicks  = 0;
    u64 endTicks    = 0;
    
    bool firstIter = true;
    while(true)
    {
        bool proceed = OS_HandleWindowEvents();
        if(!proceed) break;
        
        // In the first iteration, we simply want to render
        // the initialized state.
        if(!firstIter)
        {
            endTicks = OS_GetTicks();
            deltaTime = min(maxDeltaTime, OS_GetElapsedSeconds(startTicks, endTicks));
            startTicks = OS_GetTicks();
            
            OS_SwapBuffers();
        }
        
        MainUpdate(entities, deltaTime, &permArena, &frameArena);
        
        firstIter = false;
    }
    
    return 0;
}

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