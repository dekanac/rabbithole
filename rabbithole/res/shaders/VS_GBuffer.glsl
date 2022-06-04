#version 450

#include "common.h"

LAYOUT_IN_VEC3(0) position;
LAYOUT_IN_VEC3(1) normal;
LAYOUT_IN_VEC3(2) tangent;
LAYOUT_IN_VEC2(3) uv;

layout(location = 0) out VS_OUT 
{
    vec3 FragPos;
    vec2 FragUV;
    vec3 FragDebugOption;
    vec3 FragNormal;
    vec3 FragTangent;
    uint FragId;
    mat3 FragTBN;
    vec2 FragVelocity;
} vs_out;

//use UBO as a Constant Buffer to provide common stuff to shaders
layout(binding = 0) uniform UniformBufferObject_ 
{
    UniformBufferObject UBO;
};

layout(push_constant) uniform Push 
{
    mat4 model;
    uint id;
} push;

vec2 CalcVelocity(vec4 newPos, vec4 oldPos)
{
    oldPos.xy /= oldPos.w;
    oldPos.xy = (oldPos.xy+1)/2.0f;
    oldPos.y = 1 - oldPos.y;

    newPos.xy /= newPos.w;
    newPos.xy = (newPos.xy+1)/2.0f;
    newPos.y = 1 - newPos.y;
    
    return (newPos - oldPos).xy;
}

void main() 
{
    vec3 worldPosition =  vec3(push.model * vec4(position, 1.0));
    vs_out.FragPos = worldPosition;
    vs_out.FragUV = uv;
    
    mat3 mNormal = transpose(inverse(mat3(push.model)));

	vs_out.FragNormal = normalize(mNormal * normalize(normal));
    vs_out.FragTangent = normalize(mNormal * normalize(tangent));

	vec3 N = vs_out.FragNormal;
	vec3 T = vs_out.FragTangent;
	vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);
    
    vs_out.FragTBN = TBN;  

    vs_out.FragDebugOption = UBO.debugOption;
    
    //TODO: this works only for non moving objects, need to provide previous model matrix as well
    vec4 currentPos = UBO.viewProjMatrix * vec4(worldPosition, 1);    
    vec4 previousPos = UBO.prevViewProjMatrix * vec4(worldPosition, 1); 

    vs_out.FragVelocity = CalcVelocity(currentPos, previousPos);

    gl_Position = currentPos;
}