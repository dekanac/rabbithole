#include "common.h"

layout(binding = 0) cbuffer UniformBufferObjectBuffer 
{
    UniformBufferObject UBO;
};

[[vk::binding(1)]] Sampler3D samplerNoise3DLUT;
[[vk::binding(2)]] Sampler2D samplerDepth;

struct VolumetricCloudsParams
{
	float SunPhaseValue;
	float VolumeLightAbsorption;
	float ApproxFadeDistance;
	float CurveAggressivness;
	float EdgeGradient;
	float LightMarchSteps;
	float SomeParam1;
	float SomeParam2;
	float SomeParam3;
	float SomeParam4;
	float SomeParam5;
	float SomeParam6;
};

[[vk::binding(3)]] cbuffer VolumetricCloudsParamsBuffer
{
	VolumetricCloudsParams cloudParams;
};

[[vk::binding(4)]] cbuffer LightParamsBuffer
{
	Light lights[lightCount];
};

struct Ray
{
	float3 Origin;
	float3 Direction;
};

struct VolumeBox
{
	float3 BoundsMin;
	float3 BoundsMax;
	float3 VolumeScale;
};

VolumeBox CreateVolumeBox(float3 position, float3 scale)
{
	VolumeBox box;
	box.BoundsMin = position - scale / 2.0f;
	box.BoundsMax = position + scale / 2.0f;
	box.VolumeScale = scale;
	return box;
}

struct RaytraceResult
{
	bool Hit;
	float DistanceNear;
	float DistanceFar;
};

RaytraceResult Raytrace(Ray ray, VolumeBox box)
{
	const float3 t0 = (box.BoundsMin - ray.Origin) / ray.Direction;
	const float3 t1 = (box.BoundsMax - ray.Origin) / ray.Direction;
	
	const float3 tmin = min(t0, t1);
	const float3 tmax = max(t0, t1);

	const float dstA = max(max(tmin.x, tmin.y), tmin.z);
	const float dstB = min(min(tmax.x, tmax.y), tmax.z);

	RaytraceResult result;
	result.DistanceNear = max(0.0f, dstA);
	result.DistanceFar = max(0.0f, dstB);
	result.Hit = (dstB - result.DistanceNear) > 0.0f;
	return result;
}

float remap(float srcA, float srcB, float dstA, float dstB, float t) 
{
	return lerp(dstA, dstB, (t - srcA) / (srcB - srcA));
}

float GetTransmittance(float density)
{
	return clamp(exp(-density), 0.0f, 1.0f);
}

float3 WorldPosFromDepth(const float depth, const float2 inUV) 
{
    float4 clipSpacePosition = float4(inUV * 2.f - 1.f, depth, 1.0);
    float4 viewSpacePosition = mul(clipSpacePosition, UBO.projInverse);

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    float4 worldSpacePosition = mul(viewSpacePosition, UBO.viewInverse);

    return worldSpacePosition.xyz;
}

float DepthToDistance(float depth, float near, float far) 
{
    float linear_depth = 2.0 * near * far / (far + near - depth * (far - near));
    return linear_depth;
}

float GetDistanceFromCamera(float2 uv)
{
	//return DepthToDistance(texture(samplerDepth, uv).r, UBO.frustrumInfo.z, UBO.frustrumInfo.w);
	return distance(UBO.cameraPosition, WorldPosFromDepth(samplerDepth.Sample(uv).r, uv));
}

float GetStepNumFromDistance(float distance)
{
	return clamp(ceil(exp(-distance/cloudParams.CurveAggressivness) * cloudParams.ApproxFadeDistance), 20, 100);
}

float LightMarch(float3 position, const VolumeBox volumeBounds)
{
	const float3 toLight = normalize(lights[0].position - position);

	Ray ray;
	ray.Origin = position;
	ray.Direction = toLight;
	RaytraceResult result = Raytrace(ray, volumeBounds);

	const float insideBoxDistance = result.DistanceFar;
	const float stepSize = insideBoxDistance / cloudParams.LightMarchSteps;

	float totalDensity = 0.0f;
	for (float i = 0.0f; i < cloudParams.LightMarchSteps; i++)
	{
		const float3 samplePos = position + toLight * stepSize * i;
		totalDensity += samplerNoise3DLUT.Sample(samplePos).r * stepSize;
	}

	const float transmittance = GetTransmittance(totalDensity * cloudParams.VolumeLightAbsorption);
	return transmittance;
}

float3 WorldToLocal(float3 worldPos, float3 minBounds, float3 maxBounds)
{
    float3 localPos = (worldPos - minBounds) / (maxBounds - minBounds);
    return clamp(localPos, 0.0, 1.0);
}

float2 VolumeMarch(Ray ray, const VolumeBox volumeBounds, RaytraceResult volumeboxResult, float2 uv)
{
	const float stepNum = GetStepNumFromDistance(volumeboxResult.DistanceNear);
	const float insideBoxDistance = min(GetDistanceFromCamera(uv),volumeboxResult.DistanceFar) - volumeboxResult.DistanceNear;
	const float stepSize = insideBoxDistance / stepNum;
	
	float transmittance = 1.0f;
	float lightEnergy = 0.0f;

	for (float i = 0.0f; i < stepNum; i++)
	{
		float currentDistance = volumeboxResult.DistanceNear + i * stepSize;
		const float3 samplePos = ray.Origin + ray.Direction * currentDistance;

		const float heightPercent = (samplePos.y - volumeBounds.BoundsMin.y) / cloudParams.SomeParam3;
		const float heightGradient = clamp(remap(0.0, cloudParams.SomeParam4, 0, 1, heightPercent), 0.f, 1.f) * clamp(remap(1, cloudParams.SomeParam5, 0, 1, heightPercent), 0.f, 1.f);

        float edgeDistance = min(samplePos.x - volumeBounds.BoundsMin.x, volumeBounds.BoundsMax.x - samplePos.x);
	    edgeDistance = min(edgeDistance, min(samplePos.z - volumeBounds.BoundsMin.z, volumeBounds.BoundsMax.z - samplePos.z));
	    const float edgeGradient = min(1.0f, edgeDistance/cloudParams.EdgeGradient);

		const float gradientEffect = heightGradient * edgeGradient;
		// TODO: investigate why this has so much performance impact when SomeParam1 is smaller
        const float3 shapeSamplePos = WorldToLocal(samplePos, volumeBounds.BoundsMin, volumeBounds.BoundsMax) + cloudParams.SomeParam2;
	    const float density = samplerNoise3DLUT.Sample(shapeSamplePos).r * gradientEffect;

		if (density > 0.0f)
		{
			//TODO : include Light March instead of fixed value
			const float lightTransmittance = LightMarch(samplePos, volumeBounds);
			lightEnergy += density * stepSize * transmittance * lightTransmittance * cloudParams.SunPhaseValue;
			transmittance *= GetTransmittance(density * stepSize * cloudParams.VolumeLightAbsorption);
	
			// Ealy exit
			if (transmittance < 0.01f) break;
		}
	}

	return float2(transmittance, lightEnergy);
}

float4 main(in float2 inUV : VPOS)
{
    const VolumeBox volumeBounds1 = CreateVolumeBox(float3(2.f,   1.2f, 1.f), float3(3.f));
    const VolumeBox volumeBounds2 = CreateVolumeBox(float3(-2.f,  1.2f, 1.f), float3(3.f));
    const VolumeBox volumeBounds3 = CreateVolumeBox(float3(-7.f,  1.2f, 1.f), float3(3.f));
    const VolumeBox volumeBounds4 = CreateVolumeBox(float3(-10.f, 1.2f, 1.f), float3(3.f));
    const VolumeBox volumeBounds5 = CreateVolumeBox(float3(2.f, 35.2f, -1.f), float3(30.f, 10.f, 30.f));

	const VolumeBox volumeBoxes[5] = { volumeBounds1, volumeBounds2, volumeBounds3, volumeBounds4, volumeBounds5 };
	const uint numOfBoxes = 5;

	// Create screen ray
	const float2 uv = -(2.0f * inUV - 1.0f);
	Ray ray;
	ray.Origin = UBO.cameraPosition;
	ray.Direction = normalize(UBO.eyeXAxis.xyz * uv.xxx + UBO.eyeYAxis.xyz * uv.yyy - UBO.eyeZAxis.xyz);
	
	// Raytrace volume box
	float2 finalResult = float2(0.f);

	for (uint i = 0; i < numOfBoxes; i++) 
	{
		RaytraceResult result = Raytrace(ray, volumeBoxes[i]);
	
		// Early return if ray didn't hit the box or if volume is occluded by solid object
		if (result.Hit && (GetDistanceFromCamera(inUV) > result.DistanceNear))
    	{
    	    finalResult += VolumeMarch(ray, volumeBoxes[i], result, inUV);

			if (finalResult.y >= 1.f)
				break;
		}
    }

    return float4(1.f, 1.f, 1.f, saturate(finalResult.y));
}
