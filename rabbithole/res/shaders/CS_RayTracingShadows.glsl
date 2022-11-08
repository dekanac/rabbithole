#version 450

#include "common.h"
#include "common_raytracing.h"

layout(rgba16, binding = 4) readonly uniform image2D positionGbuffer;
layout(rgba16, binding = 5) readonly uniform image2D normalGbuffer;

layout(r8, binding = 6) writeonly uniform image2DArray outTexture;

layout(binding = 7) uniform LightParams 
{
	Light[lightCount] light;
} Lights;

layout(rgba8, binding = 8) readonly uniform image2D noiseTexture;

layout(binding = 9) uniform UniformBufferObjectBuffer 
{
    UniformBufferObject UBO;
};

const float PI = 3.14159265359;

vec3 UpVector(vec3 forward)
{
    return abs(forward.z) < 0.999f ? vec3(0.0f, 0.0f, 1.0f) : vec3(1.0f, 0.0f, 0.0f);
}

void ComputeAxes(vec3 forward, out vec3 right, out vec3 up)
{
    up    = UpVector(forward);
    right = normalize(cross(up, forward));
    up    = normalize(cross(forward, right));
}

vec3 GetPointInDisk(vec3 center, float r, vec3 forward, vec2 noise)
{
    const float theta = noise.x * 2.0f * PI;

    const float cr = sqrt(r * noise.y);

    vec3 right;
    vec3 up;
    ComputeAxes(forward, right, up);

    const float x = cr * cos(theta);
    const float y = cr * sin(theta);

    return center + (right * x) + (up * y);
}

vec2 GetNoiseFromTexture(uvec2 aPixel, uint aSeed)
{
    uvec2 t;
    t.x = aSeed * 1664525u + 1013904223u;
    t.y = t.x * (1u << 16u) + (t.x >> 16u);
    t.x += t.y * t.x;
    uvec2 uv = ((aPixel + t) & 0x3f);
    return imageLoad(noiseTexture, ivec2(uv)).xy;
}

float CalculateShadowForLight(vec3 positionOfOrigin, vec3 normalOfOrigin, Light light, uvec2 uv)
{
    float shadow = 1.f;

    if (light.radius <= 0.f || light.intensity <= 0.f)
    {
        return shadow;
    }

    vec3 lightVec = normalize(light.position - positionOfOrigin);

#ifdef SOFT_SHADOWS
    vec2 noise = GetNoiseFromTexture(uv, uint(UBO.currentFrameInfo.x));
    noise = fract(noise + (uint(UBO.currentFrameInfo.x) ) * PI);
    lightVec = normalize(GetPointInDisk(light.position.xyz , light.size, -lightVec, noise) - positionOfOrigin);
#else
    lightVec = normalize(light.position - positionOfOrigin);
#endif
    float pointToLightDistance = distance(light.position.xyz, positionOfOrigin);
    
    //if position is not facing light
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

layout( local_size_x = 8, local_size_y = 8, local_size_z = 1 ) in;
void main()
{
	vec3 worldposition = imageLoad(positionGbuffer, ivec2(gl_GlobalInvocationID.xy)).rgb;
	vec3 normalGbuffer = imageLoad(normalGbuffer, ivec2(gl_GlobalInvocationID.xy)).rgb;

	vec4 res = vec4(CalculateShadowForLight(worldposition, normalGbuffer, Lights.light[gl_GlobalInvocationID.z], gl_GlobalInvocationID.xy), 0, 0, 1);
	imageStore(outTexture, ivec3(gl_GlobalInvocationID.xyz), res);
}

