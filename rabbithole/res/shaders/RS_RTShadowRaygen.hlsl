// Copyright 2020 Google LLC

RaytracingAccelerationStructure rs : register(t0);
RWTexture2DArray<float4> image : register(u1);

RWTexture2D<float4> gbufferPosition : register(u3);
RWTexture2D<float4> gbufferNormal : register(u4);
RWTexture2D<float4> noiseTexture : register(u6);

struct UniformBufferObject
{
	float4x4 view;
	float4x4 proj;
	float3 cameraPosition;
	float3 debugOption;
	float4x4 viewProjInverse;
	float4x4 viewProjMatrix;
	float4x4 prevViewProjMatrix;
	float4x4 viewInverse;
	float4x4 projInverse;
	float4 frustrumInfo; //x = width, y = height, z = nearPlane, w = farPlane 
	float4 eyeXAxis;
	float4 eyeYAxis;
	float4 eyeZAxis;
	float4x4 projJittered;
	float4 currentFrameInfo; //x = current frame index
};

cbuffer UBO : register(b2) 
{ 
	UniformBufferObject UBO; 
};

struct Light
{
	float3 position;
	float radius;
	float3 color;
	float intensity;
	uint type;
	float size;
	float outerConeCos;
	float innerConeCos;
};

cbuffer Light : register(b5)
{
	Light lights[4];
};

struct Payload
{
	[[vk::location(0)]] float3 shadowMask;
};

static const float PI = 3.14159265f;
static const int LightType_Directional = 0;
static const int LightType_Point = 1;
static const int LightType_Spot = 2;

float2 GetNoiseFromTexture(uint2 aPixel, uint aSeed)
{
	uint2 t;
	t.x = aSeed * 1664525u + 1013904223u;
	t.y = t.x * (1u << 16u) + (t.x >> 16u);
	t.x += t.y * t.x;
	uint2 uv = ((aPixel + t) & 0x3f);
	return noiseTexture[int2(uv)].xy;
}

float3 UpVector(float3 forward)
{
	return abs(forward.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
}

void ComputeAxes(float3 forward, out float3 right, out float3 up)
{
	up = UpVector(forward);
	right = normalize(cross(up, forward));
	up = normalize(cross(forward, right));
}

float3 GetPointInDisk(float3 center, float r, float3 forward, float2 noise)
{
	const float theta = noise.x * 2.0f * PI;

	const float cr = sqrt(r * noise.y);

	float3 right;
	float3 up;
	ComputeAxes(forward, right, up);

	const float x = cr * cos(theta);
	const float y = cr * sin(theta);

	return center + (right * x) + (up * y);
}


[shader("raygeneration")]
void main()
{
	uint3 LaunchID = DispatchRaysIndex();
	uint3 LaunchSize = DispatchRaysDimensions();
	uint lightIndex = LaunchID.z;
	
	float3 lightPosition = lights[lightIndex].position;
	float3 positionOfOrigin = gbufferPosition[LaunchID.xy].xyz;
	float3 normal = gbufferNormal[LaunchID.xy].xyz;
	float3 lightVec = normalize(lightPosition - positionOfOrigin);

	// early exit if the normal of the surface doesn't face light 
	if (dot(normal, lightVec) < 0)
	{
		image[int3(LaunchID.xy, lightIndex)] = float4(0.f, 0.f, 0.f, 0.0);
		return;
	}
	
	float2 noise = GetNoiseFromTexture(LaunchID.xy, uint(UBO.currentFrameInfo.x));
    noise = frac(noise + (uint(UBO.currentFrameInfo.x) ) * PI);
	lightVec = normalize(GetPointInDisk(lights[lightIndex].position.xyz, lights[lightIndex].size, -lightVec, noise) - positionOfOrigin);
	
	float pointToLightDistance = distance(lights[lightIndex].position.xyz, positionOfOrigin);
	
	if (lights[lightIndex].type == LightType_Point && pointToLightDistance > lights[lightIndex].radius)
	{
		image[int3(LaunchID.xy, lightIndex)] = float4(0.f, 0.f, 0.f, 0.0);
		return;
	}

	RayDesc rayDesc;
	rayDesc.Origin = positionOfOrigin + normal * 0.01f;
	rayDesc.Direction = lightVec;
	rayDesc.TMin = 0.01;
	rayDesc.TMax = pointToLightDistance;

	Payload payload;
	payload.shadowMask = 1.f;
	TraceRay(rs, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, rayDesc, payload);

	image[int3(LaunchID.xy, lightIndex)] = float4(payload.shadowMask, 0.0);
}
