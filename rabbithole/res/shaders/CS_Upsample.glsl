#version 450

struct BloomParams
{
	float threshold;
    uint  texelSizeW;
    uint  texelSizeH;
    uint  mipLevel;
};

#define DOWNSAMPLE_COUNT 5

layout(binding = 0) uniform sampler2D inputImage;
layout(rgba16, binding = 1) uniform image2D outputImage;

layout(binding = 2) uniform bloomParamsBuffer
{
	BloomParams bloomParams[DOWNSAMPLE_COUNT];
};

layout(push_constant) uniform Push 
{
    uint downsampleTick;
} push;

vec4 UpsampleTent(sampler2D tex, vec2 uv, vec2 texelSize, vec4 sampleScale, uint mipLevel)
{
    vec4 d = texelSize.xyxy * vec4(1.0, 1.0, -1.0, 0.0) * sampleScale;

    vec4 s;
    s =  textureLod(tex, (uv - d.xy), mipLevel);
    s += textureLod(tex, (uv - d.wy), mipLevel) * 2.0;
    s += textureLod(tex, (uv - d.zy), mipLevel);
    s += textureLod(tex, (uv + d.zw), mipLevel) * 2.0;
    s += textureLod(tex, (uv       ), mipLevel) * 4.0;
    s += textureLod(tex, (uv + d.xw), mipLevel) * 2.0;
    s += textureLod(tex, (uv + d.zy), mipLevel);
    s += textureLod(tex, (uv + d.wy), mipLevel) * 2.0;
    s += textureLod(tex, (uv + d.xy), mipLevel);

    return s * (1.0 / 16.0);
}

layout( local_size_x = 8, local_size_y = 8, local_size_z = 1 ) in;
void main()
{
    vec2 texelSize = 1 / vec2(bloomParams[push.downsampleTick].texelSizeW, bloomParams[push.downsampleTick].texelSizeH);
    vec2 uv = gl_GlobalInvocationID.xy * texelSize;

    vec4 sampleScale = vec4(1.f);

    vec4 bloom = UpsampleTent(inputImage, uv, texelSize, sampleScale, bloomParams[push.downsampleTick].mipLevel);

    vec4 currentMip = imageLoad(outputImage, ivec2(gl_GlobalInvocationID.xy));
    
    imageStore(outputImage, ivec2(gl_GlobalInvocationID.xy), bloom + currentMip);
}