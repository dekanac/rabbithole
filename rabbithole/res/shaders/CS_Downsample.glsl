#version 450

struct BloomParams
{
	float threshold;
    uint  texelSizeW;
    uint  texelSizeH;
    uint  mipLevel;
};

layout(rgba16, binding = 0) uniform image2D inputImage;
layout(rgba16, binding = 1) uniform image2D bloomOutput;

layout(binding = 2) uniform bloomParamsBuffer
{
	BloomParams bloomParams;
};

vec4 DownsampleBox13Tap(ivec2 uv)
{
    vec4 A = imageLoad(inputImage, ivec2(uv + vec2(-1, -1)));
    vec4 B = imageLoad(inputImage, ivec2(uv + vec2( 0, -1)));
    vec4 C = imageLoad(inputImage, ivec2(uv + vec2( 1, -1)));
    //vec4 D = imageLoad(inputImage, ivec2(uv + vec2(-1, -1)));
    //vec4 E = imageLoad(inputImage, ivec2(uv + vec2( 1, -1)));
    vec4 F = imageLoad(inputImage, ivec2(uv + vec2(-1,  0)));
    vec4 G = imageLoad(inputImage, ivec2(uv)               );
    vec4 H = imageLoad(inputImage, ivec2(uv + vec2( 1,  0)));
    //vec4 I = imageLoad(inputImage, ivec2(uv + vec2(-1,  1)));
    //vec4 J = imageLoad(inputImage, ivec2(uv + vec2( 1,  1)));
    vec4 K = imageLoad(inputImage, ivec2(uv + vec2(-1,  1)));
    vec4 L = imageLoad(inputImage, ivec2(uv + vec2( 0,  1)));
    vec4 M = imageLoad(inputImage, ivec2(uv + vec2( 1,  1)));

    vec2 div = (1.0 / 4.0) * vec2(0.5, 0.125);

    vec4 o = vec4(0.f); //(D + E + I + J) * div.x;
    o += (A + B + G + F) * div.y;
    o += (B + C + H + G) * div.y;
    o += (F + G + L + K) * div.y;
    o += (G + H + M + L) * div.y;

    return o;
}

layout( local_size_x = 8, local_size_y = 8, local_size_z = 1 ) in;
void main()
{
    vec4 final = DownsampleBox13Tap(ivec2(gl_GlobalInvocationID.xy*2));
    
    imageStore(bloomOutput, ivec2(gl_GlobalInvocationID.xy), final);
}