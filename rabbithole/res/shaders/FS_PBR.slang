#include "common.h"

struct PS_IN
{
    float2 inUV : VPOS;
};

struct PS_OUT
{
    float4 outColor : SV_TARGET0;
};

[[vk::binding(0)]] Sampler2D<float4> textureAlbedo;
[[vk::binding(1)]] Sampler2D<float4> textureNormal;
[[vk::binding(2)]] Sampler2D<float4> texturePosition;
[[vk::binding(5)]] Sampler2D<float> textureSSAO;
[[vk::binding(6)]] Sampler2DArray<float4> textureShadowMap;
[[vk::binding(7)]] Sampler2D<float2> textureVelocity;
[[vk::binding(8)]] Sampler2D<float> textureDepth;
[[vk::binding(9)]] Sampler2DArray<float4> textureDenoisedShadow;
[[vk::binding(10)]] Sampler2D<float4> textureEmissive;

[[vk::binding(3)]] cbuffer UniformBufferObjectBuffer
{
    UniformBufferObject UBO;
}

[[vk::binding(4)]] cbuffer LightBuffer
{
    Light lights[lightCount];
}

#define PI (3.14159265359)

float3 WorldPosFromDepth(float2 uv, float depth) 
{
    float4 clipSpacePosition = float4(uv * 2.f - 1.f, depth, 1.0);
    float4 viewSpacePosition = mul(UBO.projInverse, clipSpacePosition);

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    float4 worldSpacePosition = mul(UBO.viewInverse, viewSpacePosition);

    return worldSpacePosition.xyz;
}

float3 SpecularReflection(MaterialInfo materialInfo, AngularInfo angularInfo)
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

float3 GetPointShade(float3 pointToLight, MaterialInfo materialInfo, float3 normal, float3 view)
{
    AngularInfo angularInfo = GetAngularInfo(pointToLight, normal, view);

    if (angularInfo.NdotL > 0.0 || angularInfo.NdotV > 0.0)
    {
        // Calculate the shading terms for the microfacet specular shading model
        float3 F = SpecularReflection(materialInfo, angularInfo);
        float Vis = VisibilityOcclusion(materialInfo, angularInfo);
        float D = MicrofacetDistribution(materialInfo, angularInfo);

        // Calculation of analytical lighting contribution
        float3 diffuseContrib = (1.0 - F) * (materialInfo.diffuseColor / PI);
        float3 specContrib = F * Vis * D;

        // Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
        return angularInfo.NdotL * (diffuseContrib + specContrib);
    }

    return float3(0.0, 0.0, 0.0);
}

float GetRangeAttenuation(float range, float distance)
{
    if (range < 0.0)
    {
        // negative range means unlimited
        return 1.0;
    }
    return max(lerp(1.0, 0.0, distance / range), 0);
    //return max(min(1.0 - pow(distance / range, 4.0), 1.0), 0.0) / pow(distance, 2.0);
}

float GetSpotAttenuation(float3 pointToLight, float3 spotDirection, float outerConeCos, float innerConeCos)
{
    float actualCos = dot(normalize(spotDirection), normalize(-pointToLight));
    if (actualCos > outerConeCos)
    {
        if (actualCos > innerConeCos)
        {
            return smoothstep(outerConeCos, innerConeCos, actualCos);
        }
        return 1.0;
    }
    return 0.0;
}

float3 ApplyPointLight(Light light, MaterialInfo materialInfo, float3 normal, float3 worldPos, float3 view)
{
    float3 pointToLight = light.position.xyz - worldPos;
    float distance = length(pointToLight);
    float attenuation = GetRangeAttenuation(light.radius, distance);
    float3 shade = GetPointShade(pointToLight, materialInfo, normal, view);
    return attenuation * light.intensity * light.color * shade;
}

float3 ApplyDirectionalLight(Light light, MaterialInfo materialInfo, float3 normal, float3 view)
{
    float3 pointToLight = light.position;
    float3 shade = GetPointShade(pointToLight, materialInfo, normal, view);
    return light.intensity * light.color * shade;
}

float3 ApplySpotLight(Light light, MaterialInfo materialInfo, float3 normal, float3 worldPos, float3 view)
{
    float3 pointToLight = light.position - worldPos;
    float distance = length(pointToLight);
    float rangeAttenuation = GetRangeAttenuation(light.radius, distance);
    float spotAttenuation = GetSpotAttenuation(pointToLight, -light.position, light.outerConeCos, light.innerConeCos);
    float3 shade = GetPointShade(pointToLight, materialInfo, normal, view);
    return rangeAttenuation * spotAttenuation * light.intensity * light.color * shade;
}

float3 DoPBRLighting(SceneInfo sceneInfo, float3 diffuseColor, float3 specularColor, float perceptualRoughness)
{
    // Roughness is authored as perceptual roughness; as is convention,
    // convert to material roughness by squaring the perceptual roughness [2].
    float alphaRoughness = perceptualRoughness * perceptualRoughness;
    
    float3 specularEnvironmentR0 = specularColor.rgb;
    // Anything less than 2% is physically impossible and is instead considered to be shadowing.
    float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
    float3 specularEnvironmentR90 = float3(1.0, 1.0, 1.0) * saturate(reflectance * 50.0);

    MaterialInfo materialInfo = (MaterialInfo)0;
    materialInfo.perceptualRoughness = perceptualRoughness;
    materialInfo.reflectance0    =  specularEnvironmentR0;
    materialInfo.alphaRoughness= alphaRoughness;
    materialInfo.diffuseColor = diffuseColor;
    materialInfo.reflectance90 = specularEnvironmentR90;
    materialInfo.specularColor= specularColor;

    float3 color = float3(0.0, 0.0, 0.0);
    float3 normal = sceneInfo.normal;
    float3 worldPos = sceneInfo.worldPos;
    float3 view = normalize(sceneInfo.cameraPosition - worldPos);

    for (int i = 0; i < lightCount; ++i)
    {
        Light light = lights[i];
        
        float shadowFactor = textureDenoisedShadow.Sample(float3(sceneInfo.uv, i)).r;
        
        if (light.type == LightType_Point)
        {
            color += ApplyPointLight(light, materialInfo, normal, worldPos, view) * shadowFactor;
        }
        else if (light.type == LightType_Directional)
        {
            color += ApplyDirectionalLight(light, materialInfo, normal, view) * shadowFactor;
        }
        else if (light.type == LightType_Spot)
        {
            color += ApplySpotLight(light, materialInfo, normal, worldPos, view) * shadowFactor;
        }
    }

    // Calculate lighting contribution from image-based lighting source (IBL)
    // ... (IBL calculation here)

    float3 emissive = float3(0, 0, 0);
    emissive = textureEmissive.Sample(sceneInfo.uv).rgb * 2.0;
    color += emissive;

    return color;   
}

PS_OUT main(PS_IN input)
{		
    float4 normalRoughness = textureNormal.Sample(input.inUV);
    float4 positionMetallic = texturePosition.Sample(input.inUV);
    float3 albedo = pow(textureAlbedo.Sample(input.inUV).rgb, float3(2.2));
    float2 velocity = textureVelocity.Sample(input.inUV).rg; 
    float ssao = textureSSAO.Sample(input.inUV).r;

    float depth = textureDepth.Sample(input.inUV).r;

    float roughness = normalRoughness.a;
    float metallic = positionMetallic.a;
    float3 position = WorldPosFromDepth(input.inUV, depth);

    float3 N = normalRoughness.rgb;
    float3 V = normalize(UBO.cameraPosition - position);

    float3 F0 = float3(0.04);
    F0 = lerp(F0, albedo, metallic);

    SceneInfo sceneInfo;
    sceneInfo.worldPos = position;
    sceneInfo.normal = N;
    sceneInfo.uv = input.inUV;
    sceneInfo.cameraPosition = UBO.cameraPosition;

    float3 LightingPBR = DoPBRLighting(sceneInfo, albedo, F0, roughness);
    
    float3 ambient = float3(0.02) * albedo * ssao;

    float3 color = ambient + LightingPBR;
    
    PS_OUT output = (PS_OUT)0;
    output.outColor = float4(color, 1.0);

    return output;
}
