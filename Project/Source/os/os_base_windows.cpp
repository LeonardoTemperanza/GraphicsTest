
#include "os_base.h"

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

void OS_DebugMessageFmt(const char* fmt, ...)
{
    // MSVC doesn't have vasprintf, so we'll have to get
    // a bit creative...
    va_list args;
    va_start(args, fmt);
    int size = vsnprintf(nullptr, 0, fmt, args);  // Extra call to get the size of the string
    va_end(args);
    
    char* str = (char*)malloc(size+1);
    defer { free(str); };
    
    va_start(args, fmt);
    vsnprintf(str, size+1, fmt, args);
    va_end(args);
    
    OutputDebugString(str);
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