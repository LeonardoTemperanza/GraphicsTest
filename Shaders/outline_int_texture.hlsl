
#pragma ps main

Texture2D<int> intTex;

float4 color : register(b0);

float4 main(float4 position: SV_POSITION) : SV_TARGET
{
    int2 pixelPos = floor(position.xy);
    int mipLevel = 0;
    int pixel = intTex.Load(int3(pixelPos, mipLevel));
    if(pixel)
        return (float4)1.0f;
    
    return (float4)1.0f;
}

/*
#version 460 core

layout(location = 0) out vec4 out_var_SV_TARGET;

uniform vec4 color;
uniform usampler2D tex;

void main()
{
    uint foundSelected = 0;
    uint foundNonSelected = 0;
    for(int y = -2; y <= 2; y++)
    {
        for(int x = -2; x <= 2; x++)
        {
            ivec2 sampleCoord = ivec2(int(floor(gl_FragCoord.x)) + x, int(floor(gl_FragCoord.y)) + y);
            uint sampled = texelFetch(tex, sampleCoord, 0).x;
            if(sampled == 0) foundNonSelected++;
            else             foundSelected++;
        }
    }
    
    float selectAmount = min(foundSelected, foundNonSelected) / 25.0f;
    out_var_SV_TARGET = vec4(color.x, color.y, color.z, color.w * selectAmount * 1.4f);
}
)";
*/