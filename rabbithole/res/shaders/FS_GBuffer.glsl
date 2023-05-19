#version 450

layout(location = 0) in VS_OUT {
    vec3 FragPos;
    vec2 FragUV;
    vec3 FragDebugOption;
    vec3 FragNormal;
    vec3 FragTangent;
    uint FragId;
    mat3 FragTBN;
    vec2 FragVelocity;
    vec4 FragEmissive;
} fs_in;

layout (location = 0) out vec4 outAlbedo;
layout (location = 1) out vec4 outNormalRoughness;
layout (location = 2) out vec4 outWorldPosMetalness;
layout (location = 3) out vec2 outVelocity;
layout (location = 4) out vec4 outEmissive;
//find solution to define things from cpp
#ifdef USE_TOOLS
layout (location = 5) out uint outEntityId;
#endif
layout (binding = 1) uniform sampler2D samplerAlbedo;
layout (binding = 2) uniform sampler2D samplerNormal;
layout (binding = 3) uniform sampler2D samplerMetalicRoughness;

layout(push_constant) uniform Push 
{
    mat4 model;
    uint id;
	bool useAlbedoMap;
	bool useNormalMap;
	bool useMetallicRoughnessMap;
    vec4 baseColor;
    vec4 emissiveColorAndStrenght;
} push;

void main() 
{
    float roughness = texture(samplerMetalicRoughness, fs_in.FragUV).g;
    float metalness = texture(samplerMetalicRoughness, fs_in.FragUV).b;
	vec3 N = normalize(fs_in.FragTBN * (texture(samplerNormal, fs_in.FragUV).xyz * 2.0 - vec3(1.0)));

    vec4 albedo = texture(samplerAlbedo, fs_in.FragUV).rgba;
    if (albedo.a <= 0.f)
        discard;
    outAlbedo = push.useAlbedoMap ? albedo : push.baseColor;
    outNormalRoughness.xyz = push.useNormalMap ? N :  fs_in.FragNormal;
    outNormalRoughness.w = push.useMetallicRoughnessMap ? roughness : 1.f;
    outWorldPosMetalness.xyz = fs_in.FragPos;
    outWorldPosMetalness.w = push.useMetallicRoughnessMap ? metalness : 1.f;

    outVelocity = fs_in.FragVelocity;
    outEmissive = fs_in.FragEmissive;

#ifdef USE_TOOLS
    outEntityId = push.id;
#endif
}