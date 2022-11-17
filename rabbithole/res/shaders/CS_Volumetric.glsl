#version 450

#include "common.h"
#include "common_raytracing.h"

#define TEX_W 160
#define TEX_H 90
#define TEX_D 128

layout(binding = 4) uniform VolumetricFogParamsBuffer
{
    VolumetricFogParams fogParams;
};

layout(r8, binding = 5) writeonly uniform image3D mediaDensity3DLUT;
layout(binding = 6) uniform sampler3D samplerNoise3DLUT;

layout(binding = 7) uniform LightParams 
{
	Light[lightCount] light;
} Lights;

layout(binding = 8) uniform UniformBufferObjectBuffer
{
    UniformBufferObject UBO;
};

const vec3 fogColorAmbient = vec3(0.5,0.5,0.5);

layout( local_size_x = 8, local_size_y = 4, local_size_z = 8 ) in;
void main()
{
    if (!bool(fogParams.isEnabled))
        return;

    vec3 screenSpacePos = gl_GlobalInvocationID.xyz * vec3(2.0f/TEX_W, 2.0f/TEX_H, 1.0f/TEX_D) + vec3(-1,-1,0);

    float origScreenSpacePosZ = screenSpacePos.z;
    screenSpacePos.z = pow(screenSpacePos.z,1.7f);
    float densityWeight = pow(origScreenSpacePosZ + 1/128.0f, 0.2f);

    uvec3 texturePos = gl_GlobalInvocationID.xyz;

    vec3 eyeRay = (UBO.eyeXAxis.xyz * screenSpacePos.xxx + UBO.eyeYAxis.xyz * screenSpacePos.yyy + UBO.eyeZAxis.xyz);

    vec3 viewRay = eyeRay * -(screenSpacePos.z * fogParams.fogDistance + fogParams.fogStartDistance);
    vec3 worldPos = UBO.cameraPosition + viewRay;

    float densityVal = 1.0f; //check shadow and put shadow factor in here
    
    vec3 sunPosition = Lights.light[0].position.xyz;
    vec3 sunLightDirection = normalize(worldPos - sunPosition);
    vec3 sunColor = Lights.light[0].color;
    
    //TODO: deal with shadows here and update densityVal
    float shadowedSunlight = 1.0f;
    float noiseDensity = texture(samplerNoise3DLUT, screenSpacePos).r * 0.2f;

    Ray ray;
    ray.origin = worldPos;
    ray.direction = normalize(sunPosition - worldPos);
    ray.t = length(sunPosition - worldPos);
    
    if (FindTriangleIntersection(ray))
    {
        shadowedSunlight = 0.4f;
        noiseDensity *= 0.3f;
    }
    else
    {
        shadowedSunlight = densityVal;
    }
    
    float verticalFalloff = 0.1f;

    float densityFinal = verticalFalloff * noiseDensity * densityWeight;

    float colorLerpFactor = 0.5f + 0.5f * dot(normalize(viewRay.xyz), sunLightDirection);

    vec3 lightIntensity = mix(fogColorAmbient, mix(sunColor, fogColorAmbient, colorLerpFactor), shadowedSunlight);

    for (int j = 0; j < 4; ++j)
    {
        if (Lights.light[j].type == LightType_Point)
        {
            vec3 incidentVector = Lights.light[j].position.xyz - worldPos;
            float distance = length(incidentVector);
            float attenutation = Lights.light[j].radius / (pow(distance, 2.0) + 1.0);
            vec3 lightColor = Lights.light[j].color;
            lightIntensity += lightColor * attenutation* attenutation;
        }
    }

    vec4 res = vec4(lightIntensity, 1.f) * densityFinal;
    imageStore(mediaDensity3DLUT, ivec3(texturePos), res);
}