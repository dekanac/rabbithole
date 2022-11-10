#version 450
#extension GL_ARB_shader_group_vote : require

#include "common.h"

layout(binding = 0) uniform UniformBufferObject_ 
{
    UniformBufferObject UBO;
};

layout (r32f, binding = 1) readonly uniform image2D imageDepth;
layout (rgba16f, binding = 2) readonly uniform image2D imageNormal;

layout(binding = 3) uniform Samples_ 
{
    vec4 samples[64];
} Samples;

layout(std140, binding = 4) uniform SSAOParamsBuffer
{
	SSAOParams ssaoParams;
};

layout (r8, binding = 5) writeonly uniform image2D SSAOOutput;

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

//instead of binding texture we can have a matrix
const vec2 NoiseMAT[4][4] = vec2[][](
	vec2[](vec2(-0.9098, -0.48026), vec2(0.32024, 0.60014), vec2(0.49988, -0.13717), vec2(-0.73401, 0.8213)),
	vec2[](vec2(0.96472, -0.63631), vec2(-0.80929, -0.47239), vec2(-0.43465, -0.70892), vec2(0.60422, -0.72786)),
	vec2[](vec2(-0.84489, 0.73858), vec2(0.25477, 0.15941), vec2(-0.98381, 0.09972), vec2(0.36057,	-0.71009)),
	vec2[](vec2(0.06787,	0.70606), vec2(-0.12267,	0.24411), vec2(-0.6009,	-0.2981), vec2(-0.724,	0.0265))
	);

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
	vec3 randomVec = normalize(vec3(NoiseMAT[gl_GlobalInvocationID.x % 4][gl_GlobalInvocationID.y % 4], 0.f));
	
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