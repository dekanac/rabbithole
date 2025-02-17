[[vk::binding(0)]] Sampler2D<float> samplerInputTexture;

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

    output.outColor = samplerInputTexture.Sample(input.inUV).r;

    return output;
}