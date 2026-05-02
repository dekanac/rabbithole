#include "common.h"

[[vk::binding(0)]] Sampler2D<float> textureSSAO;

[[vk::binding(1)]] cbuffer SSAOParamsBuffer
{
    SSAOParams ssaoParams;
};

struct VS_INPUT
{
    float2 inUV : VPOS;
};

float main(VS_INPUT input) : SV_TARGET0
{
    const int blurRange = 2;
    int n = 0;
    float2 texelSize = 1.0 / float2(ssaoParams.resWidth, ssaoParams.resHeight);

    float result = 0.f;
    for (int x = -blurRange; x < blurRange; x++) 
    {
        for (int y = -blurRange; y < blurRange; y++) 
        {
            float2 offset = float2(float(x), float(y)) * texelSize;
            result += textureSSAO.Sample(input.inUV + offset, 0).r;
            n++;
        }
    }

    return result / (float)n;
}