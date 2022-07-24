#version 450

#include "common.h"

LAYOUT_IN_VEC3(0) position;
LAYOUT_IN_VEC3(1) normal;
LAYOUT_IN_VEC3(2) tangent;
LAYOUT_IN_VEC2(3) uv;

layout(location = 0) out VS_OUT 
{
    vec3 FragPos;
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
    vs_out.FragPos = vec3(push.model * vec4(position, 1.0));   

    gl_Position = UBO.proj * UBO.view * push.model * vec4(position, 1.0);
}