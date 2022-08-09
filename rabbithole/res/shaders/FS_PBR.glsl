#version 450

#include "common.h"

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D samplerAlbedo;
layout (binding = 1) uniform sampler2D samplerNormal;
layout (binding = 2) uniform sampler2D samplerPosition;
layout (binding = 5) uniform sampler2D samplerSSAO;
layout (binding = 6) uniform sampler2DArray samplerShadowMap;
layout (binding = 7) uniform sampler2D samplerVelocity;
layout (binding = 8) uniform sampler2D samplerDepth;
layout (binding = 9) uniform sampler2D samplerDenoisedShadow;

layout (location = 0) out vec4 outColor;

layout(binding = 3) uniform UniformBufferObjectBuffer 
{
    UniformBufferObject UBO;
};

layout(std140, binding = 4) uniform LightParams 
{
	Light[lightCount] light;
} Lights;

const float PI = 3.14159265359;

vec3 WorldPosFromDepth(float depth) 
{
    vec4 clipSpacePosition = vec4(inUV * 2.f - 1.f, depth, 1.0);
    vec4 viewSpacePosition = UBO.projInverse * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = UBO.viewInverse * viewSpacePosition;

    return worldSpacePosition.xyz;
}

vec3 SpecularReflection(MaterialInfo materialInfo, AngularInfo angularInfo)
{
    return materialInfo.reflectance0 + (materialInfo.reflectance90 - materialInfo.reflectance0) * pow(clamp(1.0 - angularInfo.VdotH, 0.0, 1.0), 5.0);
}

float VisibilityOcclusion(MaterialInfo materialInfo, AngularInfo angularInfo)
{
    float NdotL = angularInfo.NdotL;
    float NdotV = angularInfo.NdotV;
    float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;

    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

float MicrofacetDistribution(MaterialInfo materialInfo, AngularInfo angularInfo)
{
    float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;
    float f = (angularInfo.NdotH * alphaRoughnessSq - angularInfo.NdotH) * angularInfo.NdotH + 1.0;
    return alphaRoughnessSq / (PI * f * f + 0.000001f);
}

vec3 GetPointShade(vec3 pointToLight, MaterialInfo materialInfo, vec3 normal, vec3 view)
{
    AngularInfo angularInfo = getAngularInfo(pointToLight, normal, view);

    if (angularInfo.NdotL > 0.0 || angularInfo.NdotV > 0.0)
    {
        // Calculate the shading terms for the microfacet specular shading model
        vec3 F = SpecularReflection(materialInfo, angularInfo);
        float Vis = VisibilityOcclusion(materialInfo, angularInfo);
        float D = MicrofacetDistribution(materialInfo, angularInfo);

        // Calculation of analytical lighting contribution
        vec3 diffuseContrib = (1.0 - F) * (materialInfo.diffuseColor / PI);
        vec3 specContrib = F * Vis * D;

        // Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
        return angularInfo.NdotL * (diffuseContrib + specContrib);
    }

    return vec3(0.0, 0.0, 0.0);
}

float GetRangeAttenuation(float range, float distance)
{
    if (range < 0.0)
    {
        // negative range means unlimited
        return 1.0;
    }
    return max(mix(1, 0, distance / range), 0);
    //return max(min(1.0 - pow(distance / range, 4.0), 1.0), 0.0) / pow(distance, 2.0);
}

vec3 ApplyPointLight(Light light, MaterialInfo materialInfo, vec3 normal, vec3 worldPos, vec3 view)
{
    vec3 pointToLight = light.position.xyz - worldPos;
    float distance = length(pointToLight);
    float attenuation = GetRangeAttenuation(light.radius, distance);
    vec3 shade = GetPointShade(pointToLight, materialInfo, normal, view);
    return attenuation * light.intensity * light.color * shade;
}

vec3 ApplyDirectionalLight(Light light, MaterialInfo materialInfo, vec3 normal, vec3 view)
{
    vec3 pointToLight = light.position; //TODO: for now we assume that direction light looks from sky to 0,0,0 coord
    vec3 shade = GetPointShade(pointToLight, materialInfo, normal, view);
    return light.intensity * light.color * shade;
}

vec3 DoPBRLighting(SceneInfo sceneInfo, in vec3 diffuseColor, in vec3 specularColor, in float perceptualRoughness)
{

    // Roughness is authored as perceptual roughness; as is convention,
    // convert to material roughness by squaring the perceptual roughness [2].
    float alphaRoughness = perceptualRoughness * perceptualRoughness;
    
    vec3 specularEnvironmentR0 = specularColor.rgb;
    // Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
    float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
    vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * clamp(reflectance * 50.0, 0.0, 1.0);

    MaterialInfo materialInfo =
    {
        perceptualRoughness,
        specularEnvironmentR0,
        alphaRoughness,
        diffuseColor,
        specularEnvironmentR90,
        specularColor
    };

    // LIGHTING

    vec3 color = vec3(0.0, 0.0, 0.0);
    vec3 normal = sceneInfo.normal;
    vec3 worldPos = sceneInfo.worldPos;
    vec3 view = normalize(sceneInfo.cameraPosition - worldPos);

    for (int i = 0; i < lightCount; ++i)
    {
        Light light = Lights.light[i];
        
        if (light.type == LightType_Point)
        {
            float shadowFactor = texture(samplerShadowMap, vec3(inUV, i)).r;
            color += ApplyPointLight(light, materialInfo, normal, worldPos, view) * shadowFactor;
        }
        else if (light.type == LightType_Directional)
        {
            float shadowFactor = texture(samplerDenoisedShadow, inUV).r;
            color += ApplyDirectionalLight(light, materialInfo, normal, view) * shadowFactor;
        }
        //else if (light.type == LightType_Spot)
        //{
        //    color += applySpotLight(light, materialInfo, normal, worldPos, view) * shadowFactor;
        //}
    }

    // Calculate lighting contribution from image based lighting source (IBL)
//#ifdef USE_IBL
//    color += getIBLContribution(materialInfo, normal, view) * perFrame.u_iblFactor * GetSSAO(Input.svPosition.xy * perFrame.u_invScreenResolution);
//#endif

    vec3 emissive = vec3(0, 0, 0);
//#ifdef ID_emissiveTexture
//    emissive = (emissiveTexture.Sample(samEmissive, getEmissiveUV(Input))).rgb * u_pbrParams.myPerObject_u_EmissiveFactor.rgb * perFrame.u_EmissiveFactor;
//#else        
//    emissive = u_pbrParams.myPerObject_u_EmissiveFactor.rgb * perFrame.u_EmissiveFactor;
//#endif
    color += emissive;

    return color;   
}

// ----------------------------------------------------------------------------
void main()
{		
    vec4 normalRoughness = texture(samplerNormal, inUV);
	vec4 positionMetallic = texture(samplerPosition, inUV);
    vec3 albedo = pow(texture(samplerAlbedo, inUV).rgb, vec3(2.2));
    vec2 velocity = texture(samplerVelocity, inUV).rg; 
    float ssao = texture(samplerSSAO, inUV).r;

    float depth = texture(samplerDepth, inUV).r;

	float roughness = normalRoughness.a;
	float metallic = positionMetallic.a;
	vec3 position = WorldPosFromDepth(depth);

    vec3 N = normalRoughness.rgb;
    vec3 V = normalize(UBO.cameraPosition - position);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    SceneInfo sceneInfo;
    sceneInfo.worldPos = position;
    sceneInfo.normal = N;
    sceneInfo.uv = inUV;
    sceneInfo.cameraPosition = UBO.cameraPosition;

    vec3 LightingPBR = DoPBRLighting(sceneInfo, albedo, F0, roughness);
    
    vec3 ambient = vec3(0.03) * albedo * ssao;

    vec3 color = ambient + LightingPBR;
 
	outColor = vec4(color, 1.0);

}