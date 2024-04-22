
@echo off
setlocal enabledelayedexpansion

if not exist ..\Build mkdir ..\Build 

pushd ..\Build

set include_dirs=/I..\Source /I..\Source\include
set lib_dirs=/LIBPATH:..\Libs

REM Use utility programs
REM utils\bin2h.exe %shader_dir%\shader.frag.spv fragShader %shader_dir%\shader.vert.spv vertShader -o ../Source/embedded_files.h

set source_files=..\Source\unity_build.cpp
set lib_files=User32.lib opengl32.lib GDI32.lib D3D11.lib dxgi.lib dxguid.lib Dwmapi.lib Shcore.lib
set output_name=graphics_test.exe

REM gfx_api could be: GFX_OPENGL, GFX_D3D12, GFX_VULKAN, etc.
set gfx_api=/DGFX_OPENGL
REM set gfx_api=
set common=/nologo /std:c++20 /FC %gfx_api% %include_dirs% %source_files% /link %lib_dirs% %lib_files% /out:%output_name% /subsystem:WINDOWS /entry:mainCRTStartup

REM Development build, debug is enabled, profiling and optimization disabled
cl /Zi /Od %common%
set build_ret=%errorlevel%

REM Uncomment to execute on every build
if %build_ret%==0 (
REM call graphics_test.exe
)

echo Done.

REM echo Importing models:

REM model_importer.exe Raptoid/Raptoid.fbx

popd