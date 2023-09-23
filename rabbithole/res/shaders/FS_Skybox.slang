[[vk::binding(1)]] SamplerCube<float4> textureSkybox;

struct VS_INPUT
{
    float4 position : SV_POSITION;
    float3 TexCoords : TEXCOORD0;
};

struct PS_OUTPUT
{
    float4 outColor : SV_TARGET0;
};

PS_OUTPUT main(VS_INPUT input)
{
    PS_OUTPUT output;

    output.outColor = textureSkybox.Sample(input.TexCoords);

    return output;
}