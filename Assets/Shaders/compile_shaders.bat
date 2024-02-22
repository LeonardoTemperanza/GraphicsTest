@echo off

glslangValidator -G -V vertex.vert -o shader.vert.spv
glslangValidator -G -V fragment.frag -o shader.frag.spv