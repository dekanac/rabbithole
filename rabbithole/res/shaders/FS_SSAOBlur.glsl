#version 450

layout(location = 0) out float outColor;
layout(location = 0) in vec2 inUV;

layout(binding = 0) uniform sampler2D samplerSSAOInput;

void main() 
{
    vec2 texelSize = 1.0 / vec2(textureSize(samplerSSAOInput, 0));
    float result = 0.0;
    for (int x = -2; x < 2; ++x) 
    {
        for (int y = -2; y < 2; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(samplerSSAOInput, inUV + offset).r;
        }
    }
    outColor = result / (4.0 * 4.0);
}  

