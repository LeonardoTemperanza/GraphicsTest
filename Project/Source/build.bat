
@echo off
setlocal enabledelayedexpansion

if not exist ..\Build mkdir ..\Build 

pushd ..\Build

if not exist ..\Source\generated mkdir ..\Source\generated

set include_dirs=/I..\Source /I..\Source\include /I..\Source\imgui
set lib_dirs=/LIBPATH:..\Libs

REM Use utility programs
REM utils\bin2h.exe %shader_dir%\shader.frag.spv fragShader %shader_dir%\shader.vert.spv vertShader -o ../Source/embedded_files.h

set source_files=..\Source\unity_build.cpp
set lib_files=User32.lib opengl32.lib GDI32.lib D3D11.lib dxgi.lib dxguid.lib Dwmapi.lib Shcore.lib Ole32.lib
set output_name=graphics_test.exe

set common=/nologo /std:c++20 /FC /W3 /we4062 /we4714 /we6340 /we6284 /we6273 /D_CRT_SECURE_NO_WARNINGS %include_dirs% %source_files% /link %lib_dirs% %lib_files% /out:%output_name%

REM Generate introspection info from the metaprogram
cl /Zi /std:c++20 /nologo /FC ..\Source\metaprogram.cpp
metaprogram

REM Uncomment to turn on sanitizer
REM set sanitizer=/fsanitize=address
set sanitizer=

REM Development build, debug is enabled, profiling and optimization disabled
cl /Zi /Od /DBoundsChecking %sanitizer% %common%
set build_ret=%errorlevel%

REM NOTE: In release, to remove the terminal, you can do: /subsystem:WINDOWS /entry:mainCRTStartup

echo Done.
popd