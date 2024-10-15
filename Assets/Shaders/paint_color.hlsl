
#pragma ps main

#include "common.hlsli"

cbuffer CodeConstants : register(CodeConstantsSlot)
{
    float4 color;
};

float4 main() : SV_TARGET
{
    return color;
}
