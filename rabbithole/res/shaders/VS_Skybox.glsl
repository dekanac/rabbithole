#version 450

#define LAYOUT_IN_VEC3(x) layout(location = x) in vec3
#define LAYOUT_IN_VEC2(x) layout(location = x) in vec2
#define LAYOUT_OUT_VEC3(x) layout(location = x) out vec3
#define LAYOUT_OUT_VEC2(x) layout(location = x) out vec2

LAYOUT_IN_VEC3(0) position;
LAYOUT_IN_VEC3(1) normal;
LAYOUT_IN_VEC3(2) tangent;
LAYOUT_IN_VEC2(3) uv;

layout(location = 0) out vec3 FragTexCoords;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
	vec3 cameraPosition;
    vec3 debugOption;
    mat4 viewProjInverse;
} UBO;

void main()
{
    FragTexCoords = position;
    vec4 pos = UBO.proj * UBO.view * vec4(position, 1.0);
    gl_Position = pos.xyww;
    gl_Position.y = -gl_Position.y;
} 