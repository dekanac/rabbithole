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
} fs_in;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outWorldPosition;
layout (location = 3) out vec2 velocity;
//find solution to define things from cpp
#ifdef USE_TOOLS
layout (location = 4) out uint outEntityId;
#endif
layout (binding = 1) uniform sampler2D albedoSampler;
layout (binding = 2) uniform sampler2D normalSampler;
layout (binding = 3) uniform sampler2D metallicRoughnessSampler;
//metalness roughness HERE

layout(push_constant) uniform Push {
    mat4 model;
    uint id;
    bool useNormalMap;
} push;

void main() 
{
    outColor = vec4(texture(albedoSampler, fs_in.FragUV).rgb, 1.0);
    float roughness = texture(metallicRoughnessSampler, fs_in.FragUV).g;
    float metalness = texture(metallicRoughnessSampler, fs_in.FragUV).b;

	vec3 N = normalize(fs_in.FragTBN * (normalize(texture(normalSampler, fs_in.FragUV).xyz * 2.0 - vec3(1.0))));
	
    outNormal = push.useNormalMap ? vec4(N, roughness) :  vec4(fs_in.FragNormal, roughness);

    outWorldPosition = vec4(fs_in.FragPos, metalness);

    //metalness roughness HERE

#ifdef USE_TOOLS
    outEntityId = push.id;
#endif

    velocity = fs_in.FragVelocity;

}