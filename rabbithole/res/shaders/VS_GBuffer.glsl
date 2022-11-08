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
layout(binding = 0) uniform UniformBufferObjectBuffer 
{
    UniformBufferObject UBO;
};

layout(push_constant) uniform Push 
{
    mat4 model;
    uint id;
} push;

void main() 
{
    vec3 worldPosition = vec3(push.model * vec4(position, 1.0));
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

    vec2 motionVector = (currentPos.xy / currentPos.w) - (previousPos.xy / previousPos.w);
    vs_out.FragVelocity = motionVector * 0.5f;

    gl_Position = UBO.projJittered * UBO.view * vec4(worldPosition, 1);
}