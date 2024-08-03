
@echo off
setlocal enabledelayedexpansion

if not exist ..\Build mkdir ..\Build 

pushd ..\Build

set include_dirs=/I..\Source /I..\Source\include /I..\Source\imgui
set lib_dirs=/LIBPATH:..\Libs

REM Use utility programs
REM utils\bin2h.exe %shader_dir%\shader.frag.spv fragShader %shader_dir%\shader.vert.spv vertShader -o ../Source/embedded_files.h

set source_files=..\Source\unity_build.cpp
set lib_files=User32.lib opengl32.lib GDI32.lib D3D11.lib dxgi.lib dxguid.lib Dwmapi.lib Shcore.lib
set output_name=graphics_test.exe

REM gfx_api could be: GFX_OPENGL, GFX_D3D12, GFX_VULKAN, etc.
set gfx_api=/DGFX_OPENGL
set common=/nologo /std:c++20 /FC /W3 /we4062 /we4714 /D_CRT_SECURE_NO_WARNINGS %gfx_api% %include_dirs% %source_files% /link %lib_dirs% %lib_files% /out:%output_name% /subsystem:WINDOWS /entry:mainCRTStartup

REM Generate introspection info from the metaprogram
cl /Zi /std:c++20 /nologo /FC ..\Source\metaprogram.cpp
metaprogram > ..\Source\generated_meta.h

REM Development build, debug is enabled, profiling and optimization disabled
cl /Zi /Od %common%
set build_ret=%errorlevel%

echo Done.
popd