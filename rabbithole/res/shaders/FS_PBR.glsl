#version 450

#include "common.h"

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D samplerAlbedo;
layout (binding = 1) uniform sampler2D samplerNormal;
layout (binding = 2) uniform sampler2D samplerposition;
layout (binding = 5) uniform sampler2D samplerSSAO;
layout (binding = 6) uniform sampler2D shadowMap;
layout (binding = 7) uniform sampler2D samplerVelocity;

layout (location = 0) out vec4 outColor;

layout(binding = 3) uniform UniformBufferObject 
{
    mat4 view;
    mat4 proj;
	vec3 cameraPosition;
    vec3 debugOption;
    mat4 viewProjInverse;
} UBO;

layout(binding = 4) uniform LightParams 
{
	Light[lightCount] light;
} Lights;

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
vec3 Uncharted2Tonemap(vec3 x)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------
void main()
{		

    vec4 normalRoughness = texture(samplerNormal, inUV);
	vec4 positionMetallic = texture(samplerposition, inUV);
    vec3 albedo = pow(texture(samplerAlbedo, inUV).rgb, vec3(2.2));
    vec2 velocity = texture(samplerVelocity, inUV).rg; 
     
    float shadows = texture(shadowMap, inUV).r;
     //HAAAACK// if normal is equal to clear value then skip, it means that we hit the skybox
     //if (normalRoughness == vec4(0.0, 0.0, 0.0, 1.0))
     //{
     ////just tonemap and gamma correct and return albedo (skybox)
     //   vec3 color = albedo;
     //   color = Uncharted2Tonemap(color * 4.5f);
	 //   color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));	
     //   // gamma correct
     //   //color = pow(color, vec3(1.0/1.6)); 
     //   outColor = vec4(color, 1.0f);
     //   return;
     //}
     
    float ssao = texture(samplerSSAO, inUV).r;

	float roughness = normalRoughness.a;
	float metallic = positionMetallic.a;
	vec3 position = positionMetallic.xyz;

    vec3 N = normalRoughness.rgb;
    vec3 V = normalize(UBO.cameraPosition - position);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < lightCount; ++i) 
    {
        // calculate per-light radiance
		vec3 tmp = Lights.light[i].position.xyz - position;
        vec3 L = normalize(tmp);
        vec3 H = normalize(V + L);
        float distance = length(tmp);
        float attenuation = Lights.light[i].radius / (pow(distance, 2.0) + 1.0);
        vec3 radiance = Lights.light[i].color * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;	  

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }   
    
    // ambient lighting (note that the next IBL tutorial will replace 
    // this ambient lighting with environment lighting).
    vec3 ambient = vec3(0.03) * albedo * ssao * shadows;

    vec3 color = ambient + Lo;

    // HDR tonemapping
    //color = color / (color + vec3(1.0));


	color = Uncharted2Tonemap(color * 4.5f);
	color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));	
    // gamma correct
    color = pow(color, vec3(1.0/1.6)); 

    if (int(UBO.debugOption.x) > 0) {
		switch (int(UBO.debugOption.x)) {
			case 1: 
				outColor.rgb = position;
				break;
			case 2: 
				outColor.rgb = N;
				break;
			case 3: 
				outColor.rgb = vec3(velocity, 0);
				break;
			case 4: 
				outColor.rgb = color;
				break;
		}		
		outColor.a = 1.0;
		return;
	}
	else 
	{
		outColor = vec4(color, 1.0);
	}

}