#version 450

#define TEX_W 160
#define TEX_H 90
#define TEX_D 64

layout(rgba16f, binding = 0) readonly uniform image3D mediaDensity3DLUT;
layout(rgba16f, binding = 1) writeonly uniform image3D scatteringTexture;

vec4 ScatterAccumulate(vec4 colorDensityFront, vec4 colorDensityBack)
{
    vec3 light = colorDensityFront.rgb + clamp(exp(-colorDensityFront.a), 0.0, 1.0) * colorDensityBack.rgb;
    return vec4(light, colorDensityFront.a + colorDensityBack.a);
}

void WriteOutput(in ivec3 pos, in vec4 value)
{
    value = vec4(value.rgb, 1 - exp(-value.a));
    value = max(value, 0);
    imageStore(scatteringTexture, pos, value);
}

layout( local_size_x = 8, local_size_y = 8, local_size_z = 1 ) in;
void main()
{
    if (gl_GlobalInvocationID.x < TEX_W && gl_GlobalInvocationID.y < TEX_H)
    {
        vec4 currentValue = imageLoad(mediaDensity3DLUT, ivec3(gl_GlobalInvocationID.xy, 0));
        vec4 nextValue = imageLoad(mediaDensity3DLUT, ivec3(gl_GlobalInvocationID.xy, 1));
        currentValue = ScatterAccumulate(currentValue, nextValue);

        WriteOutput(ivec3(gl_GlobalInvocationID.xy, 0), currentValue);

		for(uint z = 1; z < TEX_D; z++)
        {
            nextValue = imageLoad(mediaDensity3DLUT, ivec3(gl_GlobalInvocationID.xy, 2*z));
            currentValue = ScatterAccumulate(currentValue, nextValue);
            nextValue = imageLoad(mediaDensity3DLUT, ivec3(gl_GlobalInvocationID.xy, 2*z+1));
            currentValue = ScatterAccumulate(currentValue, nextValue);

            WriteOutput(ivec3(gl_GlobalInvocationID.xy, z), currentValue);
        }
    }
}