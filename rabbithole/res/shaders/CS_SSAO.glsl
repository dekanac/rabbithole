#version 450
#extension GL_ARB_shader_group_vote : require

#include "common.h"

layout(binding = 0) uniform UniformBufferObject_ 
{
    UniformBufferObject UBO;
};

layout (r32f, binding = 1) readonly uniform image2D imageDepth;
layout (rgba16f, binding = 2) readonly uniform image2D imageNormal;
layout (rg16f, binding = 3) readonly uniform image2D imageSSAONoise;

layout (r8, binding = 6) writeonly uniform image2D SSAOOutput;

layout(binding = 4) uniform Samples_ 
{
    vec4 samples[64];
} Samples;

layout(std140, binding = 5) uniform SSAOParamsBuffer
{
	SSAOParams ssaoParams;
};

vec3 WorldPosFromDepth(float depth, vec2 uv)
{
    vec4 clipSpacePosition = vec4(uv * 2.f - 1.f, depth, 1.0);
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

layout( local_size_x = 8, local_size_y = 8, local_size_z = 1 ) in;
void main()
{
	vec2 uv = gl_GlobalInvocationID.xy / vec2(ssaoParams.resWidth, ssaoParams.resHeight);

	if (!ssaoParams.ssaoOn)
	{
		imageStore(SSAOOutput, ivec2(gl_GlobalInvocationID.xy), vec4(1.f));
		return;
	}

	float depth = imageLoad(imageDepth, ivec2(gl_GlobalInvocationID.xy)).r;

	if (depth >= 1.f)
	{
		imageStore(SSAOOutput, ivec2(gl_GlobalInvocationID.xy), vec4(1.f));
		return;
	}

	// Get G-Buffer values
	vec3 fragPos = vec3(UBO.view * vec4(WorldPosFromDepth(depth, uv), 1.f));
	//vec3 normal = normalize(texture(samplerNormal, inUV).rgb * 2.0 - 1.0);

	vec3 normal = normalize(((mat3(UBO.view) * imageLoad(imageNormal, ivec2(gl_GlobalInvocationID.xy)).rgb)  * 0.5 + 0.5) * 2.0 - 1.0);

	// Get a random vector using a noise lookup
	vec3 randomVec = normalize(imageLoad(imageSSAONoise, ivec2(gl_GlobalInvocationID.xy % 4)).xyz);
	
	// Create TBN matrix
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

	// Calculate occlusion value
	float occlusion = 0.0f;
	// remove banding
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
		ivec2 offsetUV = ivec2(offset.xy * vec2(ssaoParams.resWidth, ssaoParams.resHeight));
        
		// get sample depth
        float sampleDepth = -linearize_depth(imageLoad(imageDepth, ivec2(offsetUV)).r);
        
		// range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, ssaoParams.radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + ssaoParams.bias ? 1.0 : 0.0) * rangeCheck;           
    }
	occlusion = 1.0 - (occlusion / float(ssaoParams.kernelSize));
	
	imageStore(SSAOOutput, ivec2(gl_GlobalInvocationID.xy), vec4(pow(occlusion, ssaoParams.power), 0, 0, 1));
}

