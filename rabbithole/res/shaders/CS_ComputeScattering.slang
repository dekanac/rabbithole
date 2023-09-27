#include "common.h"

[[vk::binding(0)]] Texture3D<float4> mediaDensity3DLUT;
[[vk::binding(1)]] RWTexture3D<float4> scatteringTexture;
[[vk::binding(2)]] cbuffer VolumetricFogParamsBuffer
{
    VolumetricFogParams fogParams;
};

void WriteScattering(int3 pos, float4 value)
{
    value = float4(value.rgb, 1 - exp(-value.a));
    value = max(value, 0);
    scatteringTexture[pos] = value;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (!(bool)fogParams.isEnabled)
        return;

    const uint TEX_W = fogParams.volumetricTexWidth;
    const uint TEX_H = fogParams.volumetricTexHeight;
    const uint TEX_D = fogParams.volumetricTexDepth;

    float4 finalScatter = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float previousDepthZ = 0;
    const float scatteringTextureDepth = float(TEX_D);

    for (uint z = 0; z < scatteringTextureDepth; z++)
    {
        float4 mediaDensity = mediaDensity3DLUT[uint3(dispatchThreadId.xy, z)];
        const float extinction = mediaDensity.a;

        float4 radianceAndTransmittance = float4(mediaDensity.rgb, 1.0f);
        if (extinction > 0.0f)
        {
            const float currentDepthZ = pow((z + 1) / scatteringTextureDepth, fogParams.fogDepthExponent) * fogParams.fogDistance;
            const float distanceZ = currentDepthZ - previousDepthZ;
            previousDepthZ = currentDepthZ;

            // Calculate transmittance and radiance
            float transmittance = exp(-extinction * distanceZ);
            float3 radiance = mediaDensity.rgb * (1 - transmittance) / extinction;

            radianceAndTransmittance = float4(radiance, transmittance);
        }

        finalScatter.rgb += radianceAndTransmittance.rgb * finalScatter.a;
        finalScatter.a = radianceAndTransmittance.a;

        const int3 coord = int3(dispatchThreadId.xy, z);
        if ((coord.x < TEX_W) && (coord.y < TEX_H))
        {
            WriteScattering(coord, finalScatter);
        }
    }
}
