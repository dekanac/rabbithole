#define HLSL

#include "common.h"

[[vk::binding(4)]] cbuffer VolumetricFogParamsBuffer
{
    VolumetricFogParams fogParams;
};

[[vk::binding(7)]] cbuffer LightBuffer
{
    Light lights[lightCount];
}

[[vk::binding(8)]] cbuffer UniformBufferObjectBuffer
{
    UniformBufferObject UBO;
}

[[vk::binding(5)]] RWTexture3D<float4> mediaDensity3DLUT;
[[vk::binding(6)]] Sampler3D<float> samplerNoise3DLUT;
[[vk::binding(9)]] Sampler3D<float> samplerVolumetricShadows;

[numthreads(8, 4, 8)]
void CalculateDensity(uint3 dispatchThreadId: SV_DispatchThreadID)
{
    if (!(bool)fogParams.isEnabled)
        return;

    const float TEX_W = fogParams.volumetricTexWidth;
    const float TEX_H = fogParams.volumetricTexHeight;
    const float TEX_D = fogParams.volumetricTexDepth;

    const float3 fogColorAmbient = float3(0.5, 0.5, 0.5);
    
    float3 screenSpacePos = (dispatchThreadId * float3(2.0f / TEX_W, 2.0f / TEX_H, 1.0f / TEX_D)) + float3(-1, -1, 0);
    float3 uv = (dispatchThreadId * float3(1.0f / TEX_W, 1.0f / TEX_H, 1.0f / TEX_D));

    float origScreenSpacePosZ = screenSpacePos.z;
    screenSpacePos.z = pow(screenSpacePos.z, fogParams.fogDepthExponent);
    float densityWeight = pow(origScreenSpacePosZ + 1 / 128.0f, 0.7f);

    uint3 texturePos = dispatchThreadId;

    float3 eyeRay = (UBO.eyeXAxis.xyz * screenSpacePos.xxx + UBO.eyeYAxis.xyz * screenSpacePos.yyy + UBO.eyeZAxis.xyz);

    float3 viewRay = eyeRay * -(screenSpacePos.z * fogParams.fogDistance + fogParams.fogStartDistance);
    float3 worldPos = UBO.cameraPosition + viewRay;

    float densityVal = 1.0f; // Check shadow and put shadow factor in here

    float3 sunPosition = lights[0].position.xyz;
    float3 sunLightDirection = normalize(worldPos - sunPosition);
    float3 sunColor = lights[0].color;
    float sunlightIntensity = lights[0].intensity;
    // TODO: deal with shadows here and update densityVal
    float shadowedSunlight = 1.0f;
    float noiseDensity = samplerNoise3DLUT.Sample(uv).r * 0.005f;
    float shadow = samplerVolumetricShadows.Sample(uv).r;

    if (sunlightIntensity > 0.f)
    {
        if (shadow < 1.f)
        {
            shadowedSunlight = 0.04f;
        }
        else
        {
            shadowedSunlight = densityVal;
            noiseDensity *= 60.f;
        }
    }

    float verticalFalloff = 0.1f;

    float densityFinal = verticalFalloff * noiseDensity * densityWeight;

    float colorLerpFactor = 0.5f + 0.5f * dot(normalize(viewRay.xyz), sunLightDirection);

    float3 lightIntensity = lerp(fogColorAmbient, lerp(sunColor, fogColorAmbient, colorLerpFactor), shadowedSunlight);

    for (int j = 0; j < 4; ++j)
    {
        if (lights[j].type == LightType_Point)
        {
            float3 incidentVector = lights[j].position.xyz - worldPos;
            float distance = length(incidentVector);
            float attenutation = lights[j].radius / (pow(distance, 2.0) + 1.0);
            float3 lightColor = lights[j].color;
            lightIntensity += lightColor * attenutation * attenutation;
        }
    }

    float4 res = float4(lightIntensity, 1.f) * densityFinal;
    mediaDensity3DLUT[texturePos] = res;
}

#include "common_raytracing.h"

[[vk::binding(10)]] RWTexture3D<float> imageVolumetricShadows;

[numthreads(8, 8, 1)]
void CalculateVolumetricShadows(uint3 dispatchThreadId: SV_DispatchThreadID)
{
    if (!(bool)fogParams.isEnabled)
        return;

    const float TEX_W = fogParams.volumetricTexWidth;
    const float TEX_H = fogParams.volumetricTexHeight;
    const float TEX_D = fogParams.volumetricTexDepth;

    float3 screenSpacePos = (dispatchThreadId * float3(2.0f / TEX_W, 2.0f / TEX_H, 1.0f / TEX_D)) + float3(-1, -1, 0);
    float3 uv = (dispatchThreadId * float3(1.0f / TEX_W, 1.0f / TEX_H, 1.0f / TEX_D));

    float origScreenSpacePosZ = screenSpacePos.z;
    screenSpacePos.z = pow(screenSpacePos.z, fogParams.fogDepthExponent);

    uint3 texturePos = dispatchThreadId;

    float3 eyeRay = (UBO.eyeXAxis.xyz * screenSpacePos.xxx + UBO.eyeYAxis.xyz * screenSpacePos.yyy + UBO.eyeZAxis.xyz);

    float3 viewRay = eyeRay * -(screenSpacePos.z * fogParams.fogDistance + fogParams.fogStartDistance);
    float3 worldPos = UBO.cameraPosition + viewRay;

    float3 sunPosition = lights[0].position.xyz;
    float3 sunLightDirection = normalize(worldPos - sunPosition);

    Ray ray;
    ray.origin = worldPos;
    ray.direction = normalize(sunPosition - worldPos);
    ray.t = length(sunPosition - worldPos);

    float shadow = float(!FindTriangleIntersection(ray));
    imageVolumetricShadows[texturePos] = shadow;
}