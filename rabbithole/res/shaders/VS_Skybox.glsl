#version 450

#include "common.h"

LAYOUT_IN_VEC3(0) position;
LAYOUT_IN_VEC3(1) normal;
LAYOUT_IN_VEC3(2) tangent;
LAYOUT_IN_VEC2(3) uv;

layout(location = 0) out vec3 FragTexCoords;

layout(binding = 0) uniform UniformBufferObject_ 
{
    UniformBufferObject UBO;
};

void main()
{
    FragTexCoords = position;
    vec4 pos = UBO.proj * UBO.view * vec4(position, 1.0);
    gl_Position = pos.xyww;
} 