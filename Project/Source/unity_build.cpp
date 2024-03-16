
#define UnityBuild

// Supported Graphics APIs
#ifdef _WIN32
#define OS_SupportD3D11
#define OS_SupportOpenGL
#elif defined(__linux__)
#define OS_SupportOpenGL
#elif defined(__APPLE__)
#define OS_SupportMetal
#else
#endif

#include "include/glad.c"

#include "os/os_generic.cpp"
#include "os/os_app.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "base.cpp"
#include "main.cpp"
#include "input.cpp"
#include "simulation.cpp"
#include "ui_core.cpp"
#include "asset_system.cpp"

#include "renderer/renderer_generic.cpp"
#include "renderer/renderer_opengl.cpp"
#include "renderer/renderer_d3d11.cpp"