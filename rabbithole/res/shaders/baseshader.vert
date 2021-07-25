#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 colors;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform Push {
	mat4 MVP;
	vec3 offset;
} push;

layout(location = 0) out vec3 fragcolors;

void main() 
{
  gl_Position = ubo.proj * vec4(position + push.offset, 1.0);
  fragcolors = vec3(1.f, 0.2f, 0.2f);
}