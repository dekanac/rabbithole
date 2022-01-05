
#version 450

layout (location = 0) out float fragColour;
layout (location = 0) in vec2 ex_TexCoord;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
	vec3 cameraPosition;
    vec3 debugOption;
	mat4 viewProjInverse;
} UBO;

layout (binding = 1) uniform sampler2D in_Depth;
layout (binding = 2) uniform sampler2D in_Normal;
layout (binding = 3) uniform sampler2D in_Noise;

layout(binding = 4) uniform Samples_ {
    vec3 samples[48];
} Samples;

// parameters (you'd probably want to use them as uniforms to more easily tweak the effect)
int kernelSize = 48;
float radius = 0.5;
float bias = 0.025;

// tile noise texture over screen based on screen dimensions divided by noise size
const vec2 noiseScale = vec2(1280.0/4.0, 720.0/4.0); //send theese through the UBO

vec3 reconstructVSPosFromDepth(vec2 uv)
{
  float depth = texture(in_Depth, uv).r;
  float x = uv.x * 2.0 - 1.0;
  float y = (1.0 - uv.y) * 2.0 - 1.0;
  vec4 pos = vec4(x, y, depth, 1.0);
  vec4 posVS = inverse(UBO.proj) * pos;
  return posVS.xyz / posVS.w;
}

void main()
{
    float depth = texture(in_Depth, ex_TexCoord).r;
	
	if (depth == 0.0f)
	{
		fragColour = 1.0f;
		return;
	}

	vec3 normal = normalize(texture(in_Normal, ex_TexCoord).rgb * 2.0f - 1.0f);

	vec3 posVS = reconstructVSPosFromDepth(ex_TexCoord);

	ivec2 depthTexSize = textureSize(in_Depth, 0); 
	ivec2 noiseTexSize = textureSize(in_Noise, 0);
	float renderScale = 0.5; // SSAO is rendered at 0.5x scale
	vec2 noiseUV = vec2(float(depthTexSize.x)/float(noiseTexSize.x), float(depthTexSize.y)/float(noiseTexSize.y)) * ex_TexCoord * renderScale;
	// noiseUV += vec2(0.5);
	vec3 randomVec = texture(in_Noise, noiseUV).xyz;
	
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(tangent, normal);
	mat3 TBN = mat3(tangent, bitangent, normal);

	float bias = 0.01f;

	float occlusion = 0.0f;
	int sampleCount = 0;
	for (uint i = 0; i < kernelSize; i++)
	{
		vec3 samplePos = TBN * Samples.samples[i].xyz;
		samplePos = posVS + samplePos * radius; 

		vec4 offset = vec4(samplePos, 1.0f);
		offset = UBO.proj * offset;
		offset.xy /= offset.w;
		offset.xy = offset.xy * 0.5f + 0.5f;
		offset.y = 1.0f - offset.y;
		
		vec3 reconstructedPos = reconstructVSPosFromDepth(offset.xy);
		vec3 sampledNormal = normalize(texture(in_Normal, offset.xy).xyz * 2.0f - 1.0f);
		if (dot(sampledNormal, normal) > 0.99)
		{
			++sampleCount;
		}
		else
		{
			float rangeCheck = smoothstep(0.0f, 1.0f, radius / abs(reconstructedPos.z - samplePos.z - bias));
			occlusion += (reconstructedPos.z <= samplePos.z - bias ? 1.0f : 0.0f) * rangeCheck;
			++sampleCount;
		}
	}
	occlusion = 1.0 - (occlusion / float(max(sampleCount,1)));
	
	fragColour = occlusion;
}

