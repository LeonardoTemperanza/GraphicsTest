
@echo off
setlocal enabledelayedexpansion

if not exist ..\..\Build mkdir ..\..\Build
if not exist ..\..\Build\utils mkdir ..\..\Build\utils

pushd ..\..\Build\utils

set include_dirs=/I..\..\Source /I..\..\Source\include
set lib_dirs=/LIBPATH:..\..\Libs

REM Compile utility programs
cl /nologo /std:c++20 /FC ..\..\Source\utils\bin2h.cpp /Od %include_dirs% /link /out:bin2h.exe
del bin2h.obj
cl /nologo /Od /Zi /std:c++20 /FC ..\..\Source\utils\mesh_importer.cpp %include_dirs% /link %lib_dirs% assimp-vc143-mt.lib /out:mesh_importer.exe
del mesh_importer.obj
cl /nologo /Od /Zi /std:c++20 /FC /EHsc ..\..\Source\utils\shader_importer.cpp %include_dirs% /MD /link %lib_dirs% dxcompiler.lib spirv-cross-core.lib spirv-cross-glsl.lib d3d11.lib d3dcompiler.lib /out:shader_importer.exe