#version 450

#define LAYOUT_IN_VEC3(x) layout(location = x) in vec3
#define LAYOUT_IN_VEC2(x) layout(location = x) in vec2
#define LAYOUT_OUT_VEC3(x) layout(location = x) out vec3
#define LAYOUT_OUT_VEC2(x) layout(location = x) out vec2

LAYOUT_IN_VEC3(0) position;
LAYOUT_IN_VEC3(1) normal;
LAYOUT_IN_VEC2(2) uv;

layout(location = 0) out VS_OUT {
    vec3 fragColors;
    vec3 fragNormal;
    vec3 fragPosition;
    vec2 fragUv;
    vec3 fragCameraPosition;
} vs_out;


layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} VP;

layout(push_constant) uniform Push {
    mat4 model;
	vec3 cameraPosition;
} push;


void main() 
{
  gl_Position = VP.proj * VP.view * push.model * vec4(position, 1.0);
  vs_out.fragColors = vec3(0.3f, 0.2f, 0.6f);
  vs_out.fragNormal = mat3(transpose(inverse(push.model))) * normal;
  vs_out.fragPosition = vec3(push.model * vec4(position, 1.0));
  vs_out.fragUv = uv;
  vs_out.fragCameraPosition = push.cameraPosition;
}