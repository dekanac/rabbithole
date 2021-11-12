#version 450

layout(binding = 1) uniform sampler2D inTexture;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(inTexture, inUV);
}