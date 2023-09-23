struct BloomParams
{
	float threshold;
    float knee;
    float exposure;
};

[[vk::binding(0)]] Sampler2D samplerLighting;
[[vk::binding(1)]] Sampler2D samplerBloom;
[[vk::binding(2)]] cbuffer bloomParamsBuffer
{
	BloomParams bloomParams;
};

float4 main(in float2 inUV : VPOS)
{
	float3 lighting = samplerLighting.Sample(inUV).rgb;

    lighting += samplerBloom.Sample(inUV).rgb;

    float3 color = float3(1.0f, 1.0f, 1.0f) - exp(-lighting * bloomParams.exposure);

    return float4(color, 1.0f);
}