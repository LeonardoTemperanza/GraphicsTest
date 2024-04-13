
#define UnityBuild

// Misc preprocessor directives
#if !defined(Development) && !defined(Release)
#define Development
#endif

#include "include/glad.c"

#include "os/os_base.cpp"
#include "os/os_generic.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "base.cpp"
#include "main.cpp"
#include "input.cpp"
#include "core.cpp"
#include "ui_core.cpp"
#include "asset_system.cpp"
#include "parser.cpp"

#include "renderer/renderer_generic.cpp"