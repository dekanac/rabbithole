#version 450

struct BloomParams
{
	float threshold;
    uint  texelSizeW;
    uint  texelSizeH;
    uint  mipLevel;
};

layout(rgba16, binding = 0) uniform image2D inputImage;
layout(rgba16, binding = 1) uniform image2D outputImage;

layout(binding = 2) uniform bloomParamsBuffer
{
	BloomParams bloomParams;
};

vec4 UpsampleTent(ivec2 uv)
{
    vec4 d = vec4(1, 1, -1, 0);

    vec4 s;
    s =  imageLoad(inputImage, ivec2(uv - d.xy));
    s += imageLoad(inputImage, ivec2(uv - d.wy)) * 2.0;
    s += imageLoad(inputImage, ivec2(uv - d.zy));

    s += imageLoad(inputImage, ivec2(uv + d.zw)) * 2.0;
    s += imageLoad(inputImage, ivec2(uv       )) * 4.0;
    s += imageLoad(inputImage, ivec2(uv + d.xw)) * 2.0;

    s += imageLoad(inputImage, ivec2(uv + d.zy));
    s += imageLoad(inputImage, ivec2(uv + d.wy)) * 2.0;
    s += imageLoad(inputImage, ivec2(uv + d.xy));

    return s * (1.0 / 16.0);
}

layout( local_size_x = 8, local_size_y = 8, local_size_z = 1 ) in;
void main()
{
    vec4 final = UpsampleTent(ivec2(gl_GlobalInvocationID.xy));
    
    imageStore(outputImage, ivec2(gl_GlobalInvocationID.xy), final);
}