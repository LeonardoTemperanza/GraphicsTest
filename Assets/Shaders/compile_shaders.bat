@echo off

glslangValidator -G -V blinn_phong.vert -o shader.vert.spv
glslangValidator -G -V blinn_phong.frag -o shader.frag.spv