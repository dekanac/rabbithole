#version 450

struct BloomParams
{
	float threshold;
    float knee;
    float exposure;
};

layout(location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D samplerLighting;
layout (binding = 1) uniform sampler2D samplerBloom;
layout(binding = 2) uniform bloomParamsBuffer
{
	BloomParams bloomParams;
};

layout (location = 0) out vec4 bloomOutput;

void main()
{
	vec3 lighting = texture(samplerLighting, inUV).rgb;
	
	lighting += texture(samplerBloom, inUV).rgb;

	vec3 color = vec3(1.0f, 1.0f, 1.0f) - exp(-lighting * bloomParams.exposure);

	bloomOutput = vec4(color, 1.0f);
}