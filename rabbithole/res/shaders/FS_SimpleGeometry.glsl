#version 450

layout(location = 0) in VS_OUT 
{
    vec3 FragPos;
} fs_in;

layout (location = 0) out vec4 outColor;

//metalness roughness HERE

layout(push_constant) uniform Push 
{
    mat4 model;
    uint id;
} push;

void main() 
{
    outColor = vec4(1, 0, 0, 1);
}