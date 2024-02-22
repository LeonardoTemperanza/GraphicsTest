
#include "include/glad.c"

#ifdef _WIN32
#include "os/os_windows.cpp"
#elif defined(__linux__)
#error "Linux operating system not supported."
#elif defined(__APPLE__)
#error "Apple Operating systems not supported."
#else
#error "Unknown operating system."
#endif

#include "base.cpp"
#include "main.cpp"
#include "simulation.cpp"
#include "renderer/renderer_generic.cpp"
#include "renderer/renderer_opengl.cpp"
#include "renderer/renderer_d3d11.cpp"
