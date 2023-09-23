#define HLSL

#include "common.h"

[[vk::binding(0)]] cbuffer UniformBufferObject_
{
    UniformBufferObject UBO;
};

[[vk::binding(1)]] Texture2D<float> imageDepth;
[[vk::binding(2)]] Texture2D<float4> imageNormal;

[[vk::binding(3)]] cbuffer Samples
{
    float4 samples[64];
};

[[vk::binding(4)]] cbuffer SSAOParamsBuffer
{
    SSAOParams ssaoParams;
};

[[vk::binding(5)]] RWTexture2D<float> SSAOOutput;

float3 WorldPosFromDepth(float depth, float2 uv)
{
    float4 clipSpacePosition = float4(uv * 2.f - 1.f, depth, 1.0);
    float4 viewSpacePosition = mul(UBO.projInverse, clipSpacePosition);

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    float4 worldSpacePosition = mul(UBO.viewInverse, viewSpacePosition);

    return worldSpacePosition.xyz;
}

float linearize_depth(float depth)
{
    float z = depth * 2.0f - 1.0f;
    return 2.f * (2.0f * UBO.frustrumInfo.z * UBO.frustrumInfo.w) / (UBO.frustrumInfo.w + UBO.frustrumInfo.z - z * (UBO.frustrumInfo.w - UBO.frustrumInfo.z));
}

static const float2 NoiseMAT[4][4] =
{
    { float2(-0.9098, -0.48026), float2(0.32024, 0.60014), float2(0.49988, -0.13717), float2(-0.73401, 0.8213) },
    { float2(0.96472, -0.63631), float2(-0.80929, -0.47239), float2(-0.43465, -0.70892), float2(0.60422, -0.72786) },
    { float2(-0.84489, 0.73858), float2(0.25477, 0.15941), float2(-0.98381, 0.09972), float2(0.36057, -0.71009) },
    { float2(0.06787, 0.70606), float2(-0.12267, 0.24411), float2(-0.6009, -0.2981), float2(-0.724, 0.0265) }
};

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    float2 uv = dispatchThreadID.xy / float2(ssaoParams.resWidth, ssaoParams.resHeight);

    if (!ssaoParams.ssaoOn)
    {
        SSAOOutput[dispatchThreadID.xy] = 1.f;
        return;
    }

    float depth = imageDepth.Load(int3(dispatchThreadID.xy, 0)).r;

    if (depth >= 1.f)
    {
        SSAOOutput[dispatchThreadID.xy] = 1.f;
        return;
    }

    // Get G-Buffer values
    float3 fragPos = mul(UBO.view, float4(WorldPosFromDepth(depth, uv), 1.f)).xyz;

    float3 normal = normalize(((mul((float3x3)UBO.view, imageNormal.Load(int3(dispatchThreadID.xy, 0)).rgb) * 0.5 + 0.5) * 2.0 - 1.0));

    float3 randomVec = normalize(float3(NoiseMAT[dispatchThreadID.x % 4][dispatchThreadID.y % 4], 0.f));

    float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(tangent, bitangent, normal);

    float occlusion = 0.0f;

    for (int i = 0; i < ssaoParams.kernelSize; ++i)
    {
        float3 samplePos = mul(samples[i].rgb, TBN);
        samplePos = fragPos + samplePos * ssaoParams.radius;

        float4 offset = float4(samplePos, 1.0);
        offset = mul(UBO.proj, offset);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;
        int2 offsetUV = int2(offset.xy * float2(ssaoParams.resWidth, ssaoParams.resHeight));

        float sampleDepth = -linearize_depth(imageDepth.Load(int3(offsetUV, 0)).r);

        float rangeCheck = smoothstep(0.0, 1.0, ssaoParams.radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + ssaoParams.bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / float(ssaoParams.kernelSize));

    SSAOOutput[dispatchThreadID.xy] = pow(occlusion, ssaoParams.power);
}