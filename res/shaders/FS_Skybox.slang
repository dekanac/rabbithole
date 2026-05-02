[[vk::binding(1)]] SamplerCube<float4> textureSkybox;

struct VS_INPUT
{
    float4 position : SV_POSITION;
    float3 TexCoords : TEXCOORD0;
};

float4 main(VS_INPUT input) : SV_TARGET0
{
    return textureSkybox.Sample(input.TexCoords);
}