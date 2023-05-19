#version 450

#include "common.h"

layout(binding = 0) uniform sampler3D mediaDensity3DLUT;
layout(rgba16f, binding = 1) writeonly uniform image3D scatteringTexture;

layout(binding = 2) uniform VolumetricFogParamsBuffer
{
    VolumetricFogParams fogParams;
};

void WriteScattering(in ivec3 pos, in vec4 value)
{
    value = vec4(value.rgb, 1 - exp(-value.a));
    value = max(value, 0);
    imageStore(scatteringTexture, pos, value);
}

layout( local_size_x = 8, local_size_y = 8, local_size_z = 1 ) in;
void main()
{
    if (!bool(fogParams.isEnabled))
        return;

    const float TEX_W = fogParams.volumetricTexWidth;
	const float TEX_H = fogParams.volumetricTexHeight;
	const float TEX_D = fogParams.volumetricTexDepth;

    vec2 uv = gl_GlobalInvocationID.xy / vec2(TEX_W,TEX_H);

    vec4 finalScatter =  vec4(0.0f, 0.0f, 0.0f, 1.0f);
    float previousDepthZ = 0;
    const float scatteringTextureDepth = float(TEX_D);

    for(uint z = 0; z < scatteringTextureDepth; z++)
    {
        float uvz = z / scatteringTextureDepth;
        vec4 mediaDensity = texture(mediaDensity3DLUT, vec3(uv, uvz));
        const float extinction = mediaDensity.a;

        vec4 radianceAndTransmittance = vec4(mediaDensity.rgb, 1.0f);
        if(extinction > 0.0f)
        {
            const float currentDepthZ = pow((z+1)/scatteringTextureDepth, fogParams.fogDepthExponent) * fogParams.fogDistance;
            const float distanceZ = currentDepthZ - previousDepthZ;
            previousDepthZ = currentDepthZ;
            
            //calculate transmittance and radiance
            float transmittance = exp(-extinction * distanceZ);
            vec3 radiance = mediaDensity.rgb * ( 1 - transmittance ) / extinction;

            radianceAndTransmittance = vec4(radiance, transmittance);
        }

        finalScatter.rgb += radianceAndTransmittance.rgb * finalScatter.a;
        finalScatter.a *= radianceAndTransmittance.a;

        const ivec3 coord = ivec3(gl_GlobalInvocationID.xy, z);
        if ((coord.x < TEX_W) && (coord.y < TEX_H))
        {
            WriteScattering(coord, finalScatter);
        }
    }
}