#version 450

struct BloomParams
{
	float threshold;
    uint  texelSizeW;
    uint  texelSizeH;
    uint  mipLevel;
};

#define DOWNSAMPLE_COUNT 5

layout(rgba16, binding = 0) uniform image2D inputImage;
layout(rgba16, binding = 1) uniform image2D outputImage;

layout(binding = 2) uniform bloomParamsBuffer
{
	BloomParams bloomParams[DOWNSAMPLE_COUNT];
};

layout(push_constant) uniform Push 
{
    uint downsampleTick;
} push;

// Better, temporally stable box filtering
// [Jimenez14] http://goo.gl/eomGso
// . . . . . . .
// . A . B . C .
// . . D . E . .
// . F . G . H .
// . . I . J . .
// . K . L . M .
// . . . . . . .
vec4 DownsampleBox13Tap(image2D tex, vec2 uv, vec2 texelSize)
{
   // vec4 A = texture(tex, (uv + texelSize * vec2(-1.0, -1.0)));
   // vec4 B = texture(tex, (uv + texelSize * vec2( 0.0, -1.0)));
   // vec4 C = texture(tex, (uv + texelSize * vec2( 1.0, -1.0)));
   // vec4 D = texture(tex, (uv + texelSize * vec2(-0.5, -0.5)));
   // vec4 E = texture(tex, (uv + texelSize * vec2( 0.5, -0.5)));
   // vec4 F = texture(tex, (uv + texelSize * vec2(-1.0,  0.0)));
   // vec4 G = texture(tex,  uv                                );
   // vec4 H = texture(tex, (uv + texelSize * vec2( 1.0,  0.0)));
   // vec4 I = texture(tex, (uv + texelSize * vec2(-0.5,  0.5)));
   // vec4 J = texture(tex, (uv + texelSize * vec2( 0.5,  0.5)));
   // vec4 K = texture(tex, (uv + texelSize * vec2(-1.0,  1.0)));
   // vec4 L = texture(tex, (uv + texelSize * vec2( 0.0,  1.0)));
   // vec4 M = texture(tex, (uv + texelSize * vec2( 1.0,  1.0)));
   //
   // vec2 div = (1.0 / 4.0) * vec2(0.5, 0.125);
   //
   // vec4 o = (D + E + I + J) * div.x;
   // o += (A + B + G + F) * div.y;
   // o += (B + C + H + G) * div.y;
   // o += (F + G + L + K) * div.y;
   // o += (G + H + M + L) * div.y;
   //
   // return o;
   return vec4(1.f);
}

layout( local_size_x = 8, local_size_y = 8, local_size_z = 1 ) in;
void main()
{
    vec2 texelSize = 1 / vec2(bloomParams[push.downsampleTick].texelSizeW, bloomParams[push.downsampleTick].texelSizeH);
    vec2 uv = gl_GlobalInvocationID.xy * texelSize;
    
    vec4 final = DownsampleBox13Tap(inputImage, uv, texelSize);
    
    imageStore(outputImage, ivec2(gl_GlobalInvocationID.xy), final);
}