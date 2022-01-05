#version 450

#define LAYOUT_IN_VEC3(x) layout(location = x) in vec3
#define LAYOUT_IN_VEC2(x) layout(location = x) in vec2
#define LAYOUT_OUT_VEC3(x) layout(location = x) out vec3
#define LAYOUT_OUT_VEC2(x) layout(location = x) out vec2

LAYOUT_IN_VEC3(0) position;
LAYOUT_IN_VEC3(1) normal;
LAYOUT_IN_VEC3(2) tangent;
LAYOUT_IN_VEC2(3) uv;

layout(location = 0) out VS_OUT {
    vec3 FragPos;
    vec2 FragUV;
    vec3 FragDebugOption;
    vec3 FragNormal;
    vec3 FragTangent;
    uint FragId;
} vs_out;

//use UBO as a Constant Buffer to provide common stuff to shaders
layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
	vec3 cameraPosition;
    vec3 debugOption;
    mat4 viewProjInverse;
} UBO;

layout(push_constant) uniform Push {
    mat4 model;
    uint id;
} push;

const vec3 lightPosition = vec3(0.0, 0.0, 100.0);

void main() 
{
    vs_out.FragPos = vec3(push.model * vec4(position, 1.0));   
    vs_out.FragUV = uv;
    
	vs_out.FragNormal = mat3(push.model) * normal;
	vs_out.FragTangent = vec3(mat3(push.model) * tangent.xyz);

    vs_out.FragDebugOption = UBO.debugOption;
    
    gl_Position = UBO.proj * UBO.view * push.model * vec4(position, 1.0);
    gl_Position.y = -gl_Position.y; //TODO: VULKAN INVERT Y FLIP FIX
}