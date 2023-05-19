
[[vk::binding(0)]] RaytracingAccelerationStructure rs : register(t0);
[[vk::binding(1)]] RWTexture3D<float> outputImage : register(u1);

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

struct VolumetricFogParams
{
	uint isEnabled;
	float fogAmount;
	float depthScale_debug;
	float fogStartDistance;
	float fogDistance;
	float fogDepthExponent;
	uint volumetricTexWidth;
	uint volumetricTexHeight;
	uint volumetricTexDepth;
};

[[vk::binding(2)]]
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

[[vk::binding(3)]]
cbuffer Light : register(b3)
{
	Light lights[4];
};

cbuffer FogParamsBuffer : register(b4)
{
	VolumetricFogParams fogParams;
};

struct Payload
{
	[[vk::location(0)]] float3 shadowMask;
};

static const float PI = 3.14159265f;
static const int LightType_Directional = 0;
static const int LightType_Point = 1;
static const int LightType_Spot = 2;

[shader("raygeneration")]
void main()
{
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