
#include "os_generic.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winuser.h>

#include "base.h"

bool OS_IsDebuggerPresent()
{
    return IsDebuggerPresent();
}

void* OS_MemReserve(uint64_t size)
{
    void* res = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
    assert(res && "VirtualAlloc failed.");
    if(!res) abort();
    
    return res;
}

void OS_MemCommit(void* mem, uint64_t size)
{
    void* res = VirtualAlloc(mem, size, MEM_COMMIT, PAGE_READWRITE);
    assert(res && "VirtualAlloc failed.");
    if(!res) abort();
}

void OS_MemFree(void* mem, uint64_t size)
{
    bool ok = VirtualFree(mem, 0, MEM_RELEASE);
    assert(ok && "VirtualFree failed.");
    if(!ok) abort();
}

void OS_DebugMessage(const char* message)
{
    OutputDebugString(message);
}

void OS_Sleep(uint64_t millis)
{
    Sleep(millis);
}

char* OS_GetExecutablePath()
{
    char* res = (char*)malloc(MAX_PATH);
    GetModuleFileName(nullptr, res, MAX_PATH);
    return res;
}

void OS_SetCurrentDirectory(const char* path)
{
    SetCurrentDirectory(path);
}

char* OS_GetCurrentDirectory()
{
    char* res = (char*)malloc(MAX_PATH);
    GetCurrentDirectory(MAX_PATH, res);
    return res;
}