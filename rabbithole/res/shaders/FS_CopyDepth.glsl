#version 450

layout(binding = 0) uniform sampler2D samplerInputTexture;

layout(location = 0) in vec2 inUV;

layout(location = 0) out float outColor;

void main()
{
    outColor = texture(samplerInputTexture, inUV).r;
}