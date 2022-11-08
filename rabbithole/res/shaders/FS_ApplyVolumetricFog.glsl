#version 450

#include "common.h"

#define TEX_W 160.f
#define TEX_H 90.f
#define TEX_D 64.f

layout(location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D samplerLightingMain;
layout (binding = 1) uniform sampler2D samplerDepth;
layout (binding = 2) uniform sampler3D samplerScatteringTexture;

layout (location = 0) out vec4 volumetricOutput;

layout(binding = 3) uniform UniformBufferObjectBuffer 
{
    UniformBufferObject UBO;
};

layout(binding = 4) uniform VolumetricFogParamsBuffer
{
    VolumetricFogParams fogParams;
};

float linearize_depth(float d)
{
    return UBO.frustrumInfo.z * UBO.frustrumInfo.w / (UBO.frustrumInfo.w + d * (UBO.frustrumInfo.z - UBO.frustrumInfo.w));
}

float unlinearize_depth(float c)
{
    return ((UBO.frustrumInfo.z * UBO.frustrumInfo.w) - (c * UBO.frustrumInfo.w)) / (c * (UBO.frustrumInfo.z - UBO.frustrumInfo.w));
}

void main()
{
	vec4 lightingMain = texture(samplerLightingMain, inUV);
	
	if (fogParams.isEnabled == 0)
	{
		volumetricOutput = lightingMain;
		return;
	}

	float depth = texture(samplerDepth, inUV).r;
	float linearDepth = linearize_depth(depth) * 2.f;

	float depthFinal = clamp(((linearDepth - fogParams.fogStartDistance) / fogParams.fogDistance), 0.f, 1.f);

	depthFinal = pow(depthFinal, 1/1.7f);

	vec3 linearSceneLighting = pow(lightingMain.rgb, vec3(2.2f));
	vec3 fogCoords = clamp(vec3(inUV, depthFinal), 0.0f, 1.0f);

	fogCoords = vec3(0.5f/TEX_W, 0.5f/TEX_H, 0.5f/TEX_D) + fogCoords * vec3((TEX_W-1)/TEX_W, (TEX_H-1)/TEX_H, (TEX_D-1)/TEX_D);

	vec4 fogAmount = texture(samplerScatteringTexture, fogCoords);

	vec3 val1 = mix( linearSceneLighting, fogAmount.xyz, clamp(fogAmount.a * fogParams.fogAmount, 0.0f, 1.0f));

	volumetricOutput = vec4(pow(val1,vec3(1/2.2f)), 1.0f);
}