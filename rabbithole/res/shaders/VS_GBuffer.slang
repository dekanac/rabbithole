#define HLSL

#include "common.h"

struct VS_OUT
{
    float3 FragPos : TEXCOORD0;
    float2 FragUV : TEXCOORD1;
    float3 FragDebugOption : TEXCOORD2;
    float3 FragNormal : TEXCOORD3;
    float3 FragTangent : TEXCOORD4;
    uint FragId : TEXCOORD5;
    float3 FragTBN1 : TEXCOORD6;
    float3 FragTBN2 : TEXCOORD7;
    float3 FragTBN3 : TEXCOORD8;
    float2 FragVelocity : TEXCOORD9;
    float4 FragEmissive : TEXCOORD10;
    float4 FinalPos : SV_POSITION;
};

[[vk::binding(0)]] cbuffer UniformBufferObject_
{
    UniformBufferObject UBO;
};

struct PushConstants
{
    float4x4 model;
    uint id;
    bool useAlbedoMap;
    bool useNormalMap;
    bool useMetallicRoughnessMap;
    float4 baseColor;
    float4 emissiveColorAndStrenght;
};

float3x3 Invert3x3(float3x3 mat)
{
    // Calculate the determinant of the matrix
    float det = dot(mat[0], cross(mat[1], mat[2]));

    if (abs(det) < 1e-6)
    {
        // Matrix is singular, cannot invert
        return mat; // Or you can return some default value or handle the singularity differently
    }

    // Calculate the adjugate of the matrix
    float3x3 adjugate = float3x3(
        cross(mat[1], mat[2]),
        cross(mat[2], mat[0]),
        cross(mat[0], mat[1])
    );

    // Calculate the inverse by dividing the adjugate by the determinant
    return adjugate / det;
}

[[vk::push_constant]] PushConstants Push;

VS_OUT main(in float3 position : TEXCOORD0,
            in float3 normal : TEXCOORD1,
            in float3 tangent : TEXCOORD2,
            in float2 uv : TEXCOORD3) 
{
    VS_OUT vs_out;

    float3 worldPosition = mul(Push.model, float4(position, 1.0)).xyz;
    vs_out.FragPos = worldPosition;
    vs_out.FragUV = uv;

    float3x3 mNormal = transpose(Invert3x3((float3x3)Push.model));

    vs_out.FragNormal = normalize(mul(mNormal, normalize(normal)));
    vs_out.FragTangent = normalize(mul(mNormal, normalize(tangent)));

    float3 N = vs_out.FragNormal;
    float3 T = vs_out.FragTangent;
    float3 B = cross(N, T);

    vs_out.FragTBN1 = T;
    vs_out.FragTBN2 = B;
    vs_out.FragTBN3 = N;

    vs_out.FragDebugOption = UBO.debugOption;
    vs_out.FragEmissive = Push.emissiveColorAndStrenght;

    // TODO: this works only for non-moving objects, need to provide previous model matrix as well
    float4 currentPos = mul(UBO.viewProjMatrix, float4(worldPosition, 1));
    float4 previousPos = mul(UBO.prevViewProjMatrix, float4(worldPosition, 1));

    float2 motionVector = (currentPos.xy / currentPos.w) - (previousPos.xy / previousPos.w);
    vs_out.FragVelocity = motionVector * 0.5f;

    vs_out.FragId = Push.id;
    vs_out.FinalPos = mul(mul(UBO.projJittered, UBO.view), float4(worldPosition, 1.0));
    return vs_out;
}