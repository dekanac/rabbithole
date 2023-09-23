#define HLSL

#include "common.h"

[[vk::binding(0)]] Sampler2D<float4> inputImage;
[[vk::binding(1)]] RWTexture2D<float4> outputImage;

#define EMISSIVE_CLAMP float4(1.0f, 1.0f, 1.0f, 1.0f)

struct BloomParams
{
    float knee;
    float threshold;
    float exposure;
};

[[vk::binding(2)]] cbuffer BloomParamsBuffer
{
    BloomParams bloomParams;
};

struct PushConstants
{
    uint downSampleTick;
};

[[vk::push_constant]] PushConstants Push;

float4 QuadraticThreshold(float4 color, float threshold, float3 curve)
{
    // Pixel brightness
    float br = max(max(color.r, color.g), color.b);

    // Under-threshold part: quadratic curve
    float rq = clamp(br - curve.x, 0.0f, curve.y);
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
float4 DownsampleBox13Tap(Sampler2D<float4> tex, float2 uv, float2 texelSize)
{
    float4 A = tex.Sample(uv + texelSize * float2(-1.0f, -1.0f));
    float4 B = tex.Sample(uv + texelSize * float2(0.0f, -1.0f));
    float4 C = tex.Sample(uv + texelSize * float2(1.0f, -1.0f));
    float4 D = tex.Sample(uv + texelSize * float2(-0.5f, -0.5f));
    float4 E = tex.Sample(uv + texelSize * float2(0.5f, -0.5f));
    float4 F = tex.Sample(uv + texelSize * float2(-1.0f, 0.0f));
    float4 G = tex.Sample(uv);
    float4 H = tex.Sample(uv + texelSize * float2(1.0f, 0.0f));
    float4 I = tex.Sample(uv + texelSize * float2(-0.5f, 0.5f));
    float4 J = tex.Sample(uv + texelSize * float2(0.5f, 0.5f));
    float4 K = tex.Sample(uv + texelSize * float2(-1.0f, 1.0f));
    float4 L = tex.Sample(uv + texelSize * float2(0.0f, 1.0f),);
    float4 M = tex.Sample(uv + texelSize * float2(1.0f, 1.0f));

    float2 div = (1.0f / 4.0f) * float2(0.5f, 0.125f);

    float4 o = (D + E + I + J) * div.x;
    o += (A + B + G + F) * div.y;
    o += (B + C + H + G) * div.y;
    o += (F + G + L + K) * div.y;
    o += (G + H + M + L) * div.y;

    return o;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    uint widht;
    uint height;
    inputImage.GetDimensions(widht, height);

    if (Push.downSampleTick == 1u)
    {
        const float knee = bloomParams.knee;
        const float3 curve = float3(bloomParams.threshold - knee, 2.0f * knee, 0.25f / knee);

        float2 texelSize = 1.0f / float2(uint2(widht, height) / 2 - 1);
        float2 uv = (float2)DispatchThreadID.xy * texelSize;

        float4 final = DownsampleBox13Tap(inputImage, uv, texelSize);
        final *= bloomParams.exposure;
        final = min(float4(1.0f), final);
        final = QuadraticThreshold(final, bloomParams.threshold, curve);

        outputImage[DispatchThreadID.xy] = final;
    }
    else
    {
        float2 texelSize = 1.0f / float2(uint2(widht, height) / 2 - 1);
        float2 uv = (float2)DispatchThreadID.xy * texelSize;

        float4 final = DownsampleBox13Tap(inputImage, uv, texelSize);

        outputImage[DispatchThreadID.xy] = final;
    }
}