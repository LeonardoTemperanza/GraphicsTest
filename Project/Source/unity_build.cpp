
#include "include/glad.c"

#ifdef _WIN32
#include "os/os_windows.cpp"
#elif defined(__linux__)
#error "Linux operating systems not supported."
#elif defined(__APPLE__)
#error "Apple Operating systems not supported."
#else
#error "Unknown operating system."
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "base.cpp"
#include "main.cpp"
#include "simulation.cpp"
#include "ui_core.cpp"

#include "renderer/renderer_generic.cpp"
#include "renderer/renderer_opengl.cpp"
#include "renderer/renderer_d3d11.cpp"
