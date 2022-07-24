#version 450

layout(binding = 0) uniform sampler2D samplerNoise2D;

layout(r32f, binding = 1) writeonly uniform image3D noise3DLUT;

layout( local_size_x = 1, local_size_y = 1, local_size_z = 1 ) in;
void main()
{
    ivec2 tex2dDim = textureSize(samplerNoise2D, 0);
    vec2 uv = (gl_GlobalInvocationID.xy / vec2(tex2dDim)) * vec2(2.f) - vec2(1.f);

    vec4 noiseSample = texture(samplerNoise2D, uv);
    imageStore(noise3DLUT, ivec3(gl_GlobalInvocationID.xyz), noiseSample.xxxx);
}