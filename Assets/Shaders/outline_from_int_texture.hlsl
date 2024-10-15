
#pragma ps main

#include "common.hlsli"

// This shader samples from a boolean texture and draws
// the outline between the difference of 0 and not 0
// pixels with some antialiasing

Texture2D<int> intTex : register(CodeTex0);

cbuffer CodeConstants : register(CodeConstantsSlot)
{
    float4 color;
}

float4 main(float4 position: SV_POSITION) : SV_TARGET
{
    int2 pixelPos = floor(position.xy);
    int mipLevel = 0;
    
    int numTrue = 0;
    int numFalse = 0;
    int iterCount = 0;
    for(int i = -1; i <= 1; ++i)
    {
        for(int j = -1; j <= 1; ++j)
        {
            // Third value is the mip (always 0 in our case)
            int pixel = intTex.Load(int3(pixelPos.x + i, pixelPos.y + j, 0));
            if(pixel == 0)
                ++numFalse;
            else
                ++numTrue;
            
            ++iterCount;
        }
    }
    
    float amount = min(numFalse, numTrue) / (float)iterCount;
    // TODO: Not sure why the regular value is way to dim,
    // so i multiplied it by 2.5
    return color * amount * 2.5f;
}
