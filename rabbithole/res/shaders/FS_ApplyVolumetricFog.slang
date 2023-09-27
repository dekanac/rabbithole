#include "common.h"

[[vk::binding(0)]] Sampler2D<float4> samplerLightingMain : register(t0);
[[vk::binding(1)]] Sampler2D<float> samplerDepth : register(t1);
[[vk::binding(2)]] Sampler3D<float4> samplerScatteringTexture : register(t2);

[[vk::binding(3)]] cbuffer UniformBufferObjectBuffer
{
    UniformBufferObject UBO;
}

[[vk::binding(4)]] cbuffer VolumetricFogParamsBuffer
{
    VolumetricFogParams fogParams;
};

float linearize_depth(float d)
{
    return UBO.frustrumInfo.z * (UBO.frustrumInfo.w / (UBO.frustrumInfo.w + d * (UBO.frustrumInfo.z - UBO.frustrumInfo.w)));
}

float unlinearize_depth(float c)
{
    return ((UBO.frustrumInfo.z * UBO.frustrumInfo.w) - (c * UBO.frustrumInfo.w)) / (c * (UBO.frustrumInfo.z - UBO.frustrumInfo.w));
}

float3 pow(float3 x, float3 y)
{
    return float3(pow(x.x, y.x), pow(x.y, y.y), pow(x.z, y.z));
}

float4 pow(float4 x, float4 y)
{
    return float4(pow(x.xyz, y.xyz), x.w);
}

float3 clamp01(float3 x)
{
    return clamp(x, 0.0f, 1.0f);
}

float4 clamp01(float4 x)
{
    return float4(clamp(x.xyz, 0.0f, 1.0f), x.w);
}

void main(in float2 inUV : VPOS, out float4 volumetricOutput : SV_Target0)
{
    float4 lightingMain = samplerLightingMain.Sample(inUV, 0);

    if (fogParams.isEnabled == 0)
    {
        volumetricOutput = lightingMain;
        return;
    }

    float depth = samplerDepth.Sample(inUV).r;
    float linearDepth = linearize_depth(depth) * fogParams.depthScale_debug;

    float depthFinal = clamp(((linearDepth - fogParams.fogStartDistance) / fogParams.fogDistance), 0.0f, 1.0f);

    depthFinal = pow(depthFinal, 1.0f / fogParams.fogDepthExponent);

    float3 linearSceneLighting = pow(lightingMain.rgb, float3(2.2f));
    float3 fogCoords = clamp(float3(inUV, depthFinal), 0.0f, 1.0f);

    const float TEX_W = fogParams.volumetricTexWidth;
    const float TEX_H = fogParams.volumetricTexHeight;
    const float TEX_D = fogParams.volumetricTexDepth;
    fogCoords = float3(0.5f / TEX_W, 0.5f / TEX_H, 0.5f / TEX_D) + fogCoords * float3((TEX_W - 1) / TEX_W, (TEX_H - 1) / TEX_H, (TEX_D - 1) / TEX_D);

    float4 fogAmount = samplerScatteringTexture.Sample(fogCoords);

    float3 val1 = lerp(linearSceneLighting, fogAmount.xyz, clamp(fogAmount.a * fogParams.fogAmount, 0.0f, 1.0f));

    volumetricOutput = float4(pow(val1, float3(1.0f / 2.2f)), 1.0f);
}
