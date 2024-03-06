
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

#include "renderer/renderer_generic.cpp"
#include "renderer/renderer_opengl.cpp"
#include "renderer/renderer_d3d11.cpp"