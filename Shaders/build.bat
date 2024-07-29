
@echo off

echo simple:
shader_importer simple.hlsl
echo:
echo basic_vertex
shader_importer basic_vertex.hlsl
echo:
echo screenspace_vertex
shader_importer screenspace_vertex.hlsl
echo:
echo outline_int
shader_importer outline_int_texture.hlsl
echo:

