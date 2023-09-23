struct FS_INPUT
{
    float3 FragPos : TEXCOORD0;
    float2 FragUV : TEXCOORD1;
    float3 FragDebugOption : TEXCOORD2;
    float3 FragNormal : TEXCOORD3;
    float3 FragTangent : TEXCOORD4;
    uint FragId : TEXCOORD5;
    float3 FragTBN1 : TEXCOORD6;
    float3 FragTBN2 : TEXCOORD7;
    float3 FragTBN3 : TEXCOORD8;
    float2 FragVelocity : TEXCOORD9;
    float4 FragEmissive : TEXCOORD10;
    float4 FragFinalPosition : SV_POSITION;
};

[[vk::binding(1)]] Sampler2D<float4> textureAlbedo;
[[vk::binding(2)]] Sampler2D<float4> textureNormal;
[[vk::binding(3)]] Sampler2D<float4> textureMetalicRoughness;

struct PushConstants
{
    float4x4 model;
    uint id;
    bool useAlbedoMap;
    bool useNormalMap;
    bool useMetallicRoughnessMap;
    float4 baseColor;
    float4 emissiveColorAndStrenght;
};

[[vk::push_constant]] PushConstants Push;

struct PS_OUTPUT
{
    float4 outAlbedo : SV_Target0;
    float4 outNormalRoughness : SV_Target1;
    float4 outWorldPosMetalness : SV_Target2;
    float2 outVelocity : SV_Target3;
    float4 outEmissive : SV_Target4;
#ifdef USE_TOOLS
    uint outEntityId : SV_Target5;
#endif
};

PS_OUTPUT main(FS_INPUT fs_in)
{
    PS_OUTPUT output;

    float roughness = textureMetalicRoughness.Sample(fs_in.FragUV).g;
    float metalness = textureMetalicRoughness.Sample(fs_in.FragUV).b;

    float3x3 TBN = float3x3(fs_in.FragTBN1, fs_in.FragTBN2, fs_in.FragTBN3);
    float3 N = normalize(mul(textureNormal.Sample(fs_in.FragUV).rgb * 2.f - 1.f, TBN));
    float4 albedo = textureAlbedo.Sample(fs_in.FragUV);

    if (albedo.a <= 0.f)
        discard;

    output.outAlbedo = Push.useAlbedoMap ? albedo : Push.baseColor;
    output.outNormalRoughness.xyz = Push.useNormalMap ? N :  fs_in.FragNormal;
    output.outNormalRoughness.w = Push.useMetallicRoughnessMap ? roughness : 1.f;
    output.outWorldPosMetalness.xyz = fs_in.FragPos;
    output.outWorldPosMetalness.w = Push.useMetallicRoughnessMap ? metalness : 1.f;

    output.outVelocity = fs_in.FragVelocity;
    output.outEmissive = fs_in.FragEmissive;

#ifdef USE_TOOLS
    output.outEntityId = Push.id;
#endif

    return output;
}