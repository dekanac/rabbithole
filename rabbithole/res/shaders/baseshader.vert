#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 fragColors;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPosition;


layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} VP;

layout(push_constant) uniform Push {
    mat4 model;
	vec3 offset;
} push;


void main() 
{
  gl_Position = VP.proj * VP.view * push.model * vec4(position, 1.0);
  fragColors = vec3(0.3f, 0.2f, 0.6f);
  fragNormal = normal;
  fragPosition = vec3(push.model * vec4(position, 1.0));
}