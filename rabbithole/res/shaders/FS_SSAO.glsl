
#version 450

layout (location = 0) out float outColor;
layout (location = 0) in vec2 inUV;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
	vec3 cameraPosition;
    vec3 debugOption;
} UBO;

layout (binding = 1) uniform sampler2D samplerNormal;
layout (binding = 2) uniform sampler2D samplerPosition;
layout (binding = 3) uniform sampler2D samplerNoise;

layout(binding = 4) uniform Samples_ {
    vec3 samples[48];
} Samples;

// parameters (you'd probably want to use them as uniforms to more easily tweak the effect)
int kernelSize = 48;
float radius = 0.5;
float bias = 0.025;

// tile noise texture over screen based on screen dimensions divided by noise size
const vec2 noiseScale = vec2(1280.0/4.0, 720.0/4.0); //send theese through the UBO

void main()
{
    // get input for SSAO algorithm
    vec3 fragPos = (texture(samplerPosition, inUV).xyzw * (inverse(UBO.proj))).rgb;
    vec3 normal = texture(samplerNormal, inUV).rgb;
    vec3 randomVec = normalize(texture(samplerNoise, inUV * noiseScale).xyz);
    // create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    // iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    for(int i = 0; i < kernelSize; ++i)
    {
        // get sample position
        vec3 samplePos = TBN * Samples.samples[i]; // from tangent to view-space
        samplePos = fragPos + samplePos * radius; 
        
        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePos, 1.0);
        offset = UBO.proj * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
        
        // get sample depth
        float sampleDepth = texture(samplerPosition, offset.xy).z; // get depth value of kernel sample
        
        // range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;           
    }
    occlusion = 1.0 - (occlusion / kernelSize);
    
    outColor = occlusion;
}

