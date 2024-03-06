
// This implements basic platform independent behavior
// (which the C and C++ standard libraries don't provide
// for some reason). This can be reused for other utility
// programs as well

#pragma once

#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <cstdint>

#include "base.h"

// Memory
void* OS_MemReserve(uint64_t size);
void OS_MemCommit(void* mem, uint64_t size);
void OS_MemFree(void* mem, uint64_t size);

// Paths
char* OS_GetExecutablePath();
void OS_SetCurrentDirectory(const char* path);
char* OS_GetCurrentDirectory();

// Misc
void OS_Sleep(uint64_t millis);
void OS_DebugMessage(const char* message);
bool OS_IsDebuggerPresent();