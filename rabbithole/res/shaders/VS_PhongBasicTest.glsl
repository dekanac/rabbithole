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
    vec3 fragColors;
    vec3 fragNormal;
    vec3 FragPos;
    vec3 fragTangent;
    vec2 FragUV;
    vec3 TangentViewPos;
    vec3 TangentLightPos;
    vec3 TangentFragPos;
    vec3 fragDebugOption;
} vs_out;

//use UBO as a Constant Buffer to provide common stuff to shaders
layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
	vec3 cameraPosition;
    vec3 debugOption;
} UBO;

layout(push_constant) uniform Push {
    mat4 model;
} push;

const vec3 lightPosition = vec3(0.0, 0.0, 100.0);

void main() 
{

    vs_out.FragPos = vec3(push.model * vec4(position, 1.0));   
    vs_out.FragUV = uv;
    
    mat3 normalMatrix = transpose(inverse(mat3(push.model)));
    vec3 T = normalize(normalMatrix * tangent);
    vec3 N = normalize(normalMatrix * normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    
    mat3 TBN = transpose(mat3(T, B, N));    
    vs_out.TangentLightPos = TBN * lightPosition;
    vs_out.TangentViewPos  = TBN * UBO.cameraPosition;
    vs_out.TangentFragPos  = TBN * vs_out.FragPos;

    vs_out.fragColors = vec3(0.3f, 0.2f, 0.6f);
    vs_out.fragNormal = mat3(transpose(inverse(push.model))) * normal;
    vs_out.fragTangent = tangent;
    vs_out.fragDebugOption = UBO.debugOption;
    
    gl_Position = UBO.proj * UBO.view * push.model * vec4(position, 1.0);
}