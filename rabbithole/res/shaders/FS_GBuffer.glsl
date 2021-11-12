#version 450

layout(location = 0) in VS_OUT {
    vec3 FragPos;
    vec2 FragUV;
    vec3 FragDebugOption;
    vec3 FragNormal;
    vec3 FragTangent;
} fs_in;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outWorldPosition;

layout (binding = 1) uniform sampler2D texSampler;
layout (binding = 2) uniform sampler2D normalSampler;

void main() 
{
    outColor = vec4(texture(texSampler, fs_in.FragUV).rgb, 1.0);

    vec3 N = normalize(fs_in.FragNormal);
	vec3 T = normalize(fs_in.FragTangent);
	vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);
	vec3 tnorm = TBN * normalize(texture(normalSampler, fs_in.FragUV).xyz * 2.0 - vec3(1.0));
	outNormal = vec4(tnorm, 1.0);

    outWorldPosition = vec4(fs_in.FragPos, 1.0);
}