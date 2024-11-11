
#pragma vs main

#include "common.hlsli"

float4 main(Vertex vertex) : SV_POSITION
{
    return float4(vertex.position, 1.0f);
}