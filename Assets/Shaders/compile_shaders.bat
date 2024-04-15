@echo off

REM This is for hlsl
REM ..\..\Project\Build\dxc.exe model2world_vert.hlsl -T vs_6_0 -E main -spirv -Fo model2world_vert.spv

glslangValidator -G -V blinn_phong.vert -o shader.vert.spv
glslangValidator -G -V blinn_phong.frag -o shader.frag.spv