#version 450

#include "common.h"

layout(binding = 4) uniform VolumetricFogParamsBuffer
{
    VolumetricFogParams fogParams;
};

layout(binding = 7) uniform LightParams 
{
	Light[lightCount] light;
} Lights;

layout(binding = 8) uniform UniformBufferObjectBuffer
{
    UniformBufferObject UBO;
};

const vec3 fogColorAmbient = vec3(0.5,0.5,0.5);

#ifdef MEDIA_DENSITY_CALCULATION

layout(r8, binding = 5) writeonly uniform image3D mediaDensity3DLUT;

layout(binding = 6) uniform sampler3D samplerNoise3DLUT;

layout(binding = 9) uniform sampler3D samplerVolumetricShadows;

layout( local_size_x = 8, local_size_y = 4, local_size_z = 8 ) in;
void main()
{
    if (!bool(fogParams.isEnabled))
        return;

    const float TEX_W = fogParams.volumetricTexWidth;
	const float TEX_H = fogParams.volumetricTexHeight;
	const float TEX_D = fogParams.volumetricTexDepth;

    vec3 screenSpacePos = gl_GlobalInvocationID.xyz * vec3(2.0f/TEX_W, 2.0f/TEX_H, 1.0f/TEX_D) + vec3(-1,-1,0);
    vec3 uv = gl_GlobalInvocationID.xyz * vec3(1.0f/TEX_W, 1.0f/TEX_H, 1.0f/TEX_D);

    float origScreenSpacePosZ = screenSpacePos.z;
    screenSpacePos.z = pow(screenSpacePos.z, fogParams.fogDepthExponent);
    float densityWeight = pow(origScreenSpacePosZ + 1/128.0f, 0.7f);

    uvec3 texturePos = gl_GlobalInvocationID.xyz;

    vec3 eyeRay = (UBO.eyeXAxis.xyz * screenSpacePos.xxx + UBO.eyeYAxis.xyz * screenSpacePos.yyy + UBO.eyeZAxis.xyz);

    vec3 viewRay = eyeRay * -(screenSpacePos.z * fogParams.fogDistance + fogParams.fogStartDistance);
    vec3 worldPos = UBO.cameraPosition + viewRay;

    float densityVal = 1.0f; //check shadow and put shadow factor in here
    
    vec3 sunPosition = Lights.light[0].position.xyz;
    vec3 sunLightDirection = normalize(worldPos - sunPosition);
    vec3 sunColor = Lights.light[0].color;
    float sunlightIntensity = Lights.light[0].intensity;
    //TODO: deal with shadows here and update densityVal
    float shadowedSunlight = 1.0f;
    float noiseDensity = texture(samplerNoise3DLUT, uv).r * 0.005f;
    float shadow = texture(samplerVolumetricShadows, uv).r;
    
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

    vec3 lightIntensity = mix(fogColorAmbient, mix(sunColor, fogColorAmbient, colorLerpFactor), shadowedSunlight);

    for (int j = 0; j < 4; ++j)
    {
        if (Lights.light[j].type == LightType_Point)
        {
            vec3 incidentVector = Lights.light[j].position.xyz - worldPos;
            float distance = length(incidentVector);
            float attenutation = Lights.light[j].radius / (pow(distance, 2.0) + 1.0);
            vec3 lightColor = Lights.light[j].color;
            lightIntensity += lightColor * attenutation * attenutation;
        }
    }

    vec4 res = vec4(lightIntensity, 1.f) * densityFinal;
    imageStore(mediaDensity3DLUT, ivec3(texturePos), res);
}

#endif

#ifdef COMPUTE_VOLUMETRIC_SHADOWS

#include "common_raytracing.h"

layout(binding = 10) writeonly uniform image3D imageVolumetricShadows;

layout( local_size_x = 8, local_size_y = 8, local_size_z = 1 ) in;
void main()
{
    if (!bool(fogParams.isEnabled))
        return;

    const float TEX_W = fogParams.volumetricTexWidth;
	const float TEX_H = fogParams.volumetricTexHeight;
	const float TEX_D = fogParams.volumetricTexDepth;

    vec3 screenSpacePos = gl_GlobalInvocationID.xyz * vec3(2.0f/TEX_W, 2.0f/TEX_H, 1.0f/TEX_D) + vec3(-1,-1,0);
    vec3 uv = gl_GlobalInvocationID.xyz * vec3(1.0f/TEX_W, 1.0f/TEX_H, 1.0f/TEX_D);

    float origScreenSpacePosZ = screenSpacePos.z;
    screenSpacePos.z = pow(screenSpacePos.z, fogParams.fogDepthExponent);

    uvec3 texturePos = gl_GlobalInvocationID.xyz;

    vec3 eyeRay = (UBO.eyeXAxis.xyz * screenSpacePos.xxx + UBO.eyeYAxis.xyz * screenSpacePos.yyy + UBO.eyeZAxis.xyz);

    vec3 viewRay = eyeRay * -(screenSpacePos.z * fogParams.fogDistance + fogParams.fogStartDistance);
    vec3 worldPos = UBO.cameraPosition + viewRay;

    vec3 sunPosition = Lights.light[0].position.xyz;
    vec3 sunLightDirection = normalize(worldPos - sunPosition);

    Ray ray;
    ray.origin = worldPos;
    ray.direction = normalize(sunPosition - worldPos);
    ray.t = length(sunPosition - worldPos);

    imageStore(imageVolumetricShadows, ivec3(texturePos), vec4(!FindTriangleIntersection(ray), 0, 0, 0));
}

#endif