#version 450

#include "common.h"

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
    uint downsampleTick;
} push;

vec4 QuadraticThreshold(vec4 color, float threshold, vec3 curve)
{
    // Pixel brightness
    float br = max(max(color.r, color.g), color.b);

    // Under-threshold part: quadratic curve
    float rq = clamp(br - curve.x, 0.0, curve.y);
    rq = curve.z * rq * rq;

    // Combine and apply the brightness response curve.
    color *= max(rq, br - threshold) / max(br, EPSILON);

    return color;
}

// Better, temporally stable box filtering
// [Jimenez14] http://goo.gl/eomGso
// . . . . . . .
// . A . B . C .
// . . D . E . .
// . F . G . H .
// . . I . J . .
// . K . L . M .
// . . . . . . .
vec4 DownsampleBox13Tap(sampler2D tex, vec2 uv, vec2 texelSize)
{
   vec4 A = texture(tex, (uv + texelSize * vec2(-1.0, -1.0)));
   vec4 B = texture(tex, (uv + texelSize * vec2( 0.0, -1.0)));
   vec4 C = texture(tex, (uv + texelSize * vec2( 1.0, -1.0)));
   vec4 D = texture(tex, (uv + texelSize * vec2(-0.5, -0.5)));
   vec4 E = texture(tex, (uv + texelSize * vec2( 0.5, -0.5)));
   vec4 F = texture(tex, (uv + texelSize * vec2(-1.0,  0.0)));
   vec4 G = texture(tex,  uv                                );
   vec4 H = texture(tex, (uv + texelSize * vec2( 1.0,  0.0)));
   vec4 I = texture(tex, (uv + texelSize * vec2(-0.5,  0.5)));
   vec4 J = texture(tex, (uv + texelSize * vec2( 0.5,  0.5)));
   vec4 K = texture(tex, (uv + texelSize * vec2(-1.0,  1.0)));
   vec4 L = texture(tex, (uv + texelSize * vec2( 0.0,  1.0)));
   vec4 M = texture(tex, (uv + texelSize * vec2( 1.0,  1.0)));
   
   vec2 div = (1.0 / 4.0) * vec2(0.5, 0.125);
   
   vec4 o = (D + E + I + J) * div.x;
   o += (A + B + G + F) * div.y;
   o += (B + C + H + G) * div.y;
   o += (F + G + L + K) * div.y;
   o += (G + H + M + L) * div.y;
   
   return o;
}

const vec4 EMISSIVE_CLAMP = vec4(1.0f); // too much fireflies if not clamped to 1 //TODO: investigate

layout( local_size_x = 8, local_size_y = 8, local_size_z = 1 ) in;
void main()
{
    if (push.downsampleTick == 1)
    {
        const float knee = bloomParams.knee;
        const vec3 curve = vec3(bloomParams.threshold  - knee, 2.0f * knee, 0.25f / knee);
    
        vec2 texelSize = 1.f / vec2(textureSize(inputImage, 0) / 2 - 1);
        vec2 uv = gl_GlobalInvocationID.xy * texelSize;
    
        vec4 final = DownsampleBox13Tap(inputImage, uv, texelSize);
        final *= bloomParams.exposure;
        final = min(EMISSIVE_CLAMP, final);
        final = QuadraticThreshold(final, bloomParams.threshold, curve);
    
        imageStore(outputImage, ivec2(gl_GlobalInvocationID.xy), final);
    }
    else
    {
        vec2 texelSize = 1.f / vec2(textureSize(inputImage, 0) / 2 - 1);
        vec2 uv = gl_GlobalInvocationID.xy * texelSize;
        
        vec4 final = DownsampleBox13Tap(inputImage, uv, texelSize);
        
        imageStore(outputImage, ivec2(gl_GlobalInvocationID.xy), final);
    }
}