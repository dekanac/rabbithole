#version 450

#include "common.h"

LAYOUT_IN_VEC3(0) position;
LAYOUT_IN_VEC3(1) normal;
LAYOUT_IN_VEC3(2) tangent;
LAYOUT_IN_VEC2(3) uv;

layout(location = 0) out VS_OUT 
{
    vec3 WorldPos;
    vec3 FragNormal;
} vs_out;

layout(push_constant) uniform Push 
{
    mat4 model;
    uint id;
	bool useAlbedoMap;
	bool useNormalMap;
	bool useMetallicRoughnessMap;
    vec4 baseColor;
    vec4 emissiveColorAndStrenght;
} push;

layout(binding = 0) uniform UniformBufferObjectBuffer 
{
    UniformBufferObject UBO;
};

void main() 
{
    vec3 worldPosition = vec3(push.model * vec4(position, 1.0));
    vs_out.WorldPos = worldPosition;
    
    mat3 mNormal = transpose(inverse(mat3(push.model)));

	vs_out.FragNormal = normalize(mNormal * normalize(normal));

    gl_Position = UBO.projJittered * UBO.view * vec4(worldPosition, 1);
}