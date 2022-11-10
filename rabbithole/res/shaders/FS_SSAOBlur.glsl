#version 450

#include "common.h"

layout(location = 0) out float outColor;
layout(location = 0) in vec2 inUV;

layout(binding = 0) uniform sampler2D samplerSSAOInput;

layout(std140, binding = 1) uniform SSAOParamsBuffer
{
	SSAOParams ssaoParams;
};

void main() 
{
	const int blurRange = 2;
	int n = 0;
	vec2 texelSize = 1.0 / vec2(ssaoParams.resWidth, ssaoParams.resHeight);
	
	float result = 0.0;
	for (int x = -blurRange; x < blurRange; x++) 
	{
		for (int y = -blurRange; y < blurRange; y++) 
		{
			vec2 offset = vec2(float(x), float(y)) * texelSize;
			result += texture(samplerSSAOInput, inUV + offset).r;
			n++;
		}
	}

	outColor = result / (float(n));
} 
