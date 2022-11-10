
#version 450

#include "common.h"

layout (location = 0) out float fragColour;
layout (location = 0) in vec2 inUV;

layout(binding = 0) uniform UniformBufferObject_ 
{
    UniformBufferObject UBO;
};

layout (binding = 1) uniform sampler2D samplerDepth;
layout (binding = 2) uniform sampler2D samplerNormal;
layout (binding = 3) uniform sampler2D samplerSSAONoise;

layout(binding = 4) uniform Samples_ 
{
    vec4 samples[64];
} Samples;

layout(std140, binding = 5) uniform SSAOParamsBuffer
{
	SSAOParams ssaoParams;
};

vec3 WorldPosFromDepth(float depth) 
{
    vec4 clipSpacePosition = vec4(inUV * 2.f - 1.f, depth, 1.0);
    vec4 viewSpacePosition = UBO.projInverse * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = UBO.viewInverse * viewSpacePosition;

    return worldSpacePosition.xyz;
}

float linearize_depth(float depth)
{
	float z = depth * 2.0f - 1.0f; 
	return 2.f * (2.0f * UBO.frustrumInfo.z * UBO.frustrumInfo.w) / (UBO.frustrumInfo.w + UBO.frustrumInfo.z - z * (UBO.frustrumInfo.w - UBO.frustrumInfo.z));	
}

// tile noise texture over screen based on screen dimensions divided by noise size
vec2 noiseScale = vec2(ssaoParams.resWidth/4.0, ssaoParams.resHeight/4.0);

void main()
{
	if (!ssaoParams.ssaoOn)
	{
		fragColour = 1.0f;
		return;
	}

	float depth = texture(samplerDepth, inUV).r;
	if (depth >= 1.f)
	{
		fragColour = 1.0f;
		return;
	}
	// Get G-Buffer values
	vec3 fragPos = vec3(UBO.view * vec4(WorldPosFromDepth(depth), 1.f));
	//vec3 normal = normalize(texture(samplerNormal, inUV).rgb * 2.0 - 1.0);

	vec3 normal = normalize(((mat3(UBO.view) * texture(samplerNormal, inUV).rgb)  * 0.5 + 0.5) * 2.0 - 1.0);

	// Get a random vector using a noise lookup
	vec3 randomVec = normalize(texture(samplerSSAONoise, inUV * noiseScale).xyz);
	
	// Create TBN matrix
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

	// Calculate occlusion value
	float occlusion = 0.0f;
	// remove banding
#pragma unroll_loop_start
	for(int i = 0; i < ssaoParams.kernelSize; ++i)
    {
        // get sample position
        vec3 samplePos = TBN * Samples.samples[i].rgb; // from tangent to view-space
        samplePos = fragPos + samplePos * ssaoParams.radius; 
        
        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePos, 1.0);
        offset = UBO.proj * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
        
        // get sample depth
        float sampleDepth = -linearize_depth(texture(samplerDepth, offset.xy).r);
        
        // range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, ssaoParams.radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + ssaoParams.bias ? 1.0 : 0.0) * rangeCheck;           
    }
#pragma unroll_loop_end
	occlusion = 1.0 - (occlusion / float(ssaoParams.kernelSize));
	
	fragColour = pow(occlusion, ssaoParams.power);
}

