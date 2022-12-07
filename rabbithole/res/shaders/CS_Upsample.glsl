#version 450

struct BloomParams
{
	float threshold;
    float knee;
    float exposure;
};

layout(binding = 0) uniform sampler2D inputImage;
layout(rgba16, binding = 1) uniform image2D outputImage;

layout(binding = 2) uniform bloomParamsBuffer
{
	BloomParams bloomParams;
};

layout(push_constant) uniform Push 
{
    uint isLastPass;
} push;

vec4 UpsampleTent(sampler2D tex, vec2 uv, vec2 texelSize, vec4 sampleScale)
{
    vec4 d = texelSize.xyxy * vec4(1.0, 1.0, -1.0, 0.0) * sampleScale;

    vec4 s;
    s =  texture(tex, (uv - d.xy));
    s += texture(tex, (uv - d.wy)) * 2.0;
    s += texture(tex, (uv - d.zy));
    s += texture(tex, (uv + d.zw)) * 2.0;
    s += texture(tex, (uv       )) * 4.0;
    s += texture(tex, (uv + d.xw)) * 2.0;
    s += texture(tex, (uv + d.zy));
    s += texture(tex, (uv + d.wy)) * 2.0;
    s += texture(tex, (uv + d.xy));

    return s * (1.0 / 16.0);
}

layout( local_size_x = 8, local_size_y = 8, local_size_z = 1 ) in;
void main()
{
    vec2 texelSize = 1.f / vec2(textureSize(inputImage, 0));
    vec2 uv = (gl_GlobalInvocationID.xy/2 + 0.5f) * texelSize;

    vec4 sampleScale = vec4(1.f);

    vec4 bloom = UpsampleTent(inputImage, uv, texelSize, sampleScale);
    
    vec4 currentMip = vec4(0.0);

    if (!bool(push.isLastPass))
        currentMip = imageLoad(outputImage, ivec2(gl_GlobalInvocationID.xy));
    
    imageStore(outputImage, ivec2(gl_GlobalInvocationID.xy), bloom + currentMip);
}