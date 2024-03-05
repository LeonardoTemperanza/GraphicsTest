
@echo off
setlocal enabledelayedexpansion

if not exist ..\Build mkdir ..\Build 

pushd ..\Build

set include_dirs=/I..\Source /I..\Source\include
set lib_dirs=/LIBPATH:..\Libs

set shader_dir=..\..\Assets\Shaders

REM Compile Shaders
pushd %shader_dir%
call compile_shaders.bat
popd

REM Compile utility programs
cl /nologo /std:c++20 ..\Source\utils\bin2h.cpp /Od %include_dirs% /link /out:bin2h.exe
del bin2h.obj
cl /nologo /Od /std:c++20 /FC ..\Source\utils\model_importer.cpp %include_dirs% /link %lib_dirs% assimp-vc143-mt.lib /out:model_importer.exe

REM Use utility programs
bin2h.exe %shader_dir%\shader.frag.spv fragShader %shader_dir%\shader.vert.spv vertShader -o ../Source/embedded_files.h

REM Cleanup utility programs
del bin2h.obj bin2h.exe

set source_files=..\Source\unity_build.cpp
set lib_files=User32.lib opengl32.lib GDI32.lib D3D11.lib dxgi.lib dxguid.lib Dwmapi.lib Shcore.lib
set output_name=graphics_test.exe
set gfx_api=/DFORCE_OPENGL
REM set gfx_api=
set common=/nologo /std:c++20 /FC %gfx_api% %include_dirs% %source_files% /link %lib_dirs% %lib_files% /out:%output_name% /subsystem:WINDOWS /entry:mainCRTStartup

REM Development build, debug is enabled, profiling and optimization disabled
cl /Zi /Od %common%
set build_ret=%errorlevel%

REM Uncomment to execute on every build
if %build_ret%==0 (
REM call graphics_test.exe
)

popd