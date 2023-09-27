#include "common.h"

[[vk::binding(0)]] RaytracingAccelerationStructure rs;
[[vk::binding(1)]] RWTexture3D<float> outputImage;

[[vk::binding(2)]]
cbuffer UniformBufferObjectBuffer
{
	UniformBufferObject UBO;
};

[[vk::binding(3)]]
cbuffer Light
{
	Light lights[4];
};

[[vk::binding(4)]] cbuffer FogParamsBuffer
{
	VolumetricFogParams fogParams;
};

struct Payload
{
	[[vk::location(0)]] float3 shadowMask;
};

#define PI (3.14159265f)

[shader("raygeneration")]
void main()
{
	if (!(bool)fogParams.isEnabled)
	{
		return;
	}
	
	uint3 LaunchID = DispatchRaysIndex();
	uint3 LaunchSize = DispatchRaysDimensions();
	
	const float TEX_W = fogParams.volumetricTexWidth;
	const float TEX_H = fogParams.volumetricTexHeight;
	const float TEX_D = fogParams.volumetricTexDepth;
	float3 screenSpacePos = LaunchID * float3(2.0f / TEX_W, 2.0f / TEX_H, 1.0f / TEX_D) + float3(-1, -1, 0);

	float origScreenSpacePosZ = screenSpacePos.z;
	screenSpacePos.z = pow(screenSpacePos.z, fogParams.fogDepthExponent); //fog depth exponent here

	uint3 texturePos = LaunchSize;

	float3 eyeRay = (UBO.eyeXAxis.xyz * screenSpacePos.xxx + UBO.eyeYAxis.xyz * screenSpacePos.yyy + UBO.eyeZAxis.xyz);

	float3 viewRay = eyeRay * -(screenSpacePos.z * fogParams.fogDistance + fogParams.fogStartDistance);
	float3 worldPos = UBO.cameraPosition + viewRay;
    
	float3 sunPosition = lights[0].position.xyz;
	float3 sunLightDirection = normalize(worldPos - sunPosition);
    
	RayDesc rayDesc;
	rayDesc.Origin = worldPos;
	rayDesc.Direction = normalize(sunPosition - worldPos);
	rayDesc.TMin = 0.01;
	rayDesc.TMax = length(sunPosition - worldPos);

	Payload payload;
	payload.shadowMask = 1.f;
	TraceRay(rs, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, rayDesc, payload);

	outputImage[int3(LaunchID)] = payload.shadowMask.x;
}