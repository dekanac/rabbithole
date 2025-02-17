#include "common.h"
#include "common_raytracing.h"

[[vk::binding(4)]] Texture2D<float4> positionGbuffer;
[[vk::binding(5)]] Texture2D<float4> normalGbuffer;
[[vk::binding(6)]] RWTexture2DArray<float> outTexture;

[[vk::binding(7)]] cbuffer LightBuffer 
{ 
    Light lights[lightCount]; 
}
[[vk::binding(8)]] Texture2D<float4> noiseTexture;

[[vk::binding(9)]] cbuffer UniformBufferObjectBuffer 
{ 
    UniformBufferObject UBO; 
}

#define PI (3.14159265359)

// Helper function to calculate the up vector.
float3 UpVector(float3 forward)
{
    return abs(forward.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
}

// Helper function to compute the right and up vectors from a forward vector.
void ComputeAxes(float3 forward, out float3 right, out float3 up)
{
    up = UpVector(forward);
    right = normalize(cross(up, forward));
    up = normalize(cross(forward, right));
}

// Helper function to get a point in a disk.
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

// Helper function to get noise from a texture.
float2 GetNoiseFromTexture(uint2 aPixel, uint aSeed)
{
    uint2 t;
    t.x = aSeed * 1664525u + 1013904223u;
    t.y = t.x * (1u << 16u) + (t.x >> 16u);
    t.x += t.y * t.x;
    uint2 uv = ((aPixel + t) & uint2(0x3f, 0x3f));
    return noiseTexture[int2(uv)].xy;
}

// Function to calculate shadow for a light.
float CalculateShadowForLight(float3 positionOfOrigin, float3 normalOfOrigin, Light light, uint2 uv)
{
    float shadow = 1.0f;

    if (light.radius <= 0.0f || light.intensity <= 0.0f)
    {
        return shadow;
    }

    float3 lightVec = normalize(light.position - positionOfOrigin);
    float pointToLightDistance = distance(light.position.xyz, positionOfOrigin);

#ifdef SOFT_SHADOWS
    float2 noise = GetNoiseFromTexture(uv, (uint)UBO.currentFrameInfo.x);
    noise = frac(noise + (uint(UBO.currentFrameInfo.x)) * PI);
    lightVec = normalize(GetPointInDisk(light.position.xyz, light.size, -lightVec, noise) - positionOfOrigin);
#endif

    // If position is not facing the light.
    if (dot(normalOfOrigin, lightVec) < 0)
    {
        return IN_SHADOW;
    }

    if (light.type == LightType_Point && pointToLightDistance > light.radius)
    {
        return IN_SHADOW;
    }

    Ray ray;
    ray.origin = positionOfOrigin + normalOfOrigin * 0.01;
    ray.direction = lightVec;
    ray.t = pointToLightDistance;

    if (FindTriangleIntersection(ray))
        shadow = IN_SHADOW;

    return shadow;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId: SV_DispatchThreadID)
{
    float3 worldposition = positionGbuffer.Load(int3(dispatchThreadId.xy, 0)).xyz;
    float3 normal = normalGbuffer.Load(int3(dispatchThreadId.xy, 0)).xyz;

    outTexture[dispatchThreadId.xyz] = CalculateShadowForLight(worldposition, normal, lights[dispatchThreadId.z], dispatchThreadId.xy);
}
