#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 colors;

layout(push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 projectionMatrix;
	vec3 offset;
} push;

layout(location = 0) out vec3 fragcolors;

void main() 
{
  gl_Position = push.projectionMatrix * push.viewMatrix * push.modelMatrix * vec4(position + push.offset, 1.0);
  fragcolors = colors;
}