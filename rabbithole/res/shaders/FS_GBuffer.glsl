#version 450

layout(location = 0) in VS_OUT {
    vec3 FragPos;
    vec2 FragUV;
    vec3 FragDebugOption;
    vec3 FragNormal;
    vec3 FragTangent;
    uint FragId;
} fs_in;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outWorldPosition;
layout (location = 3) out uint outEntityId;

layout (binding = 1) uniform sampler2D albedoSampler;
layout (binding = 2) uniform sampler2D normalSampler;
layout (binding = 3) uniform sampler2D roughnessSampler;
layout (binding = 4) uniform sampler2D metalnessSampler;
//metalness roughness HERE

layout(push_constant) uniform Push {
    mat4 model;
    uint id;
} push;

void main() 
{
    outColor = vec4(texture(albedoSampler, fs_in.FragUV).rgb, 1.0);
    float roughness = texture(roughnessSampler, fs_in.FragUV).r;
    float metalness = texture(metalnessSampler, fs_in.FragUV).r;

    vec3 N = normalize(fs_in.FragNormal);
	vec3 T = normalize(fs_in.FragTangent);
	vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);
	vec3 tnorm = normalize(TBN * (texture(normalSampler, fs_in.FragUV).xyz * 2.0 - vec3(1.0)));
	outNormal = vec4(tnorm, roughness);

    outWorldPosition = vec4(fs_in.FragPos, metalness);

    //metalness roughness HERE

    outEntityId = push.id;
}