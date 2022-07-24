#version 450

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec3 TexCoords;

layout (binding = 1) uniform samplerCube samplerSkybox;

void main()
{    
    outColor = texture(samplerSkybox, TexCoords);
}