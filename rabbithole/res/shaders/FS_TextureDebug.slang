#define HLSL

#include "common.h"

[[vk::binding(0)]] cbuffer DebugTextureParams
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
};

[[vk::binding(1)]] Sampler2D samplerInput2D;
[[vk::binding(2)]] Sampler2DArray samplerInput2DArray;
[[vk::binding(3)]] Sampler3D samplerInput3D;

float4 main(in float2 inUV: VPOS)
{
	float4 inputTex;
    if (isArray)
        inputTex = samplerInput2DArray.Sample(float3(inUV, arraySlice));
    else if (is3D)
        inputTex = samplerInput3D.Sample(float3(inUV, texture3DDepthScale));
	else
		inputTex = samplerInput2D.Sample(inUV);

    float4 res = float4(0,0,0,1);

	if (showR)
		res.r = inputTex.r;

	if (showG)
		res.g = inputTex.g;

	if (showB)
		res.b = inputTex.b;

	if (showA)
		res.a = inputTex.a;

	return res;
}