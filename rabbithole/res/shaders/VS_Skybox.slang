#define HLSL

#include "common.h"

struct VS_INPUT
{
    float3 position : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 tangent : TEXCOORD2;
    float2 uv : TEXCOORD3;
};

struct VS_OUTPUT
{
    float3 FragTexCoords : VPOS;
    float4 SV_Position : SV_POSITION;
};

[[vk::binding(0)]] cbuffer UniformBufferObjectBuffer
{
    UniformBufferObject UBO;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    output.FragTexCoords = input.position;
    float4 pos = mul(mul(UBO.projJittered, UBO.view), float4(input.position, 1.0));
    output.SV_Position = float4(pos.xy, pos.ww);

    return output;
}