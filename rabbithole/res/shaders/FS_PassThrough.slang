[[vk::binding(0)]]Texture2D<float4> samplerInputTexture;
[[vk::binding(1)]]SamplerState samplerInputTextureSampler;

struct VS_INPUT
{
    float2 inUV : VPOS;
};

struct PS_OUTPUT
{
    float4 outColor : SV_TARGET0;
};

PS_OUTPUT main(VS_INPUT input)
{
    PS_OUTPUT output;

    output.outColor = samplerInputTexture.Sample(samplerInputTextureSampler, input.inUV);

    return output;
}