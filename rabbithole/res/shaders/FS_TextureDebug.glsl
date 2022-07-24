#version 450

#include "common.h"

layout(std140, binding = 0) uniform DebugTextureParamsBuffer 
{
	bool hasMips;
	int mipSlice;
	int mipCount;

	bool isArray;
	int arraySlice;
	int arrayCount;
	
	bool showR;
	bool showG;
	bool showB;
	bool showA;

	bool is3D;
	float texture3DDepthScale;

} DebugTextureParams;

layout(location = 0) in vec2 inUV;

layout(binding = 1) uniform sampler2D samplerInput2D;
layout(binding = 2) uniform sampler2DArray samplerInput2DArray;
layout(binding = 3) uniform sampler3D samplerInput3D;

layout(location = 0) out vec4 outTexture;

void main()
{
	vec4 inputTex;
	if (DebugTextureParams.isArray)
		inputTex = texture(samplerInput2DArray, vec3(inUV, DebugTextureParams.arraySlice));
	else if (DebugTextureParams.is3D)
		inputTex = texture(samplerInput3D, vec3(inUV, DebugTextureParams.texture3DDepthScale));
	else
		inputTex = texture(samplerInput2D, inUV);

	vec4 res = vec4(0,0,0,1);

	if (DebugTextureParams.showR)
		res.r = inputTex.r;

	if (DebugTextureParams.showG)
		res.g = inputTex.g;

	if (DebugTextureParams.showB)
		res.b = inputTex.b;

	if (DebugTextureParams.showA)
		res.a = inputTex.a;

	outTexture = res;
}