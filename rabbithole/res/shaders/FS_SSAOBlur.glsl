#version 450

layout(location = 0) out float outColor;
layout(location = 0) in vec2 inUV;

layout(binding = 0) uniform sampler2D samplerSSAOInput;

layout(push_constant) uniform Push 
{
    uint direction;
} push;

layout(std140, binding = 1) uniform SSAOParamsBuffer
{
	float radius;
	float bias;
	float resWidth;
	float resHeight;
	float power;
	uint kernelSize;
	bool ssaoOn;
} SSAOParams;

float blur13(sampler2D image, vec2 uv, vec2 resolution, vec2 direction) 
{
    float color = 0.0;
    
    vec2 off1 = vec2(1.411764705882353) * direction;
    vec2 off2 = vec2(3.2941176470588234) * direction;
    vec2 off3 = vec2(5.176470588235294) * direction;

    color += texture(image, uv).r * 0.1964825501511404;
    color += texture(image, uv + (off1 / resolution)).r * 0.2969069646728344;
    color += texture(image, uv - (off1 / resolution)).r * 0.2969069646728344;
    color += texture(image, uv + (off2 / resolution)).r * 0.09447039785044732;
    color += texture(image, uv - (off2 / resolution)).r * 0.09447039785044732;
    color += texture(image, uv + (off3 / resolution)).r * 0.010381362401148057;
    color += texture(image, uv - (off3 / resolution)).r * 0.010381362401148057;
    
    return color;
}

void main() 
{
    
    vec2 direction = vec2(push.direction == 0.f ? 1.f : 0.f, push.direction == 1.f ? 1.f : 0.f);
    outColor = blur13(samplerSSAOInput, inUV, vec2(SSAOParams.resWidth, SSAOParams.resHeight), direction);
} 
