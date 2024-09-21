
#define UnityBuild

#include "metaprogram_custom_keywords.h"

// Misc preprocessor directives
#if !defined(Development) && !defined(Release)
#define Development
#endif

#include "include/glad.c"

// Generated code
#include "generated/introspection.cpp"

// Main Project
#include "os/os_generic.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "base.cpp"
#include "main.cpp"
#include "input.cpp"
#include "core.cpp"
#include "editor.cpp"
#include "asset_system.cpp"
#include "collision.cpp"
#include "renderer/renderer_generic.cpp"
#include "sound/sound_generic.cpp"

// Libraries
#include "imgui/imgui.cpp"
#include "imgui/imgui_demo.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_tables.cpp"
#include "imgui/imgui_widgets.cpp"

#ifdef GFX_OPENGL
#include "imgui/backends/imgui_impl_opengl3.cpp"
#else
#error "TODO"
#endif

#ifdef _WIN32
#include "imgui/backends/imgui_impl_win32.cpp"
#elif defined(__linux__)
#error "Linux operating systems not supported."
#elif defined(__APPLE__)
#error "Apple Operating systems not supported."
#else
#error "Unknown operating system."
#endif