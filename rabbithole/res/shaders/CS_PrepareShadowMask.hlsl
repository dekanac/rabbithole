/**********************************************************************
Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
RTShadowDenoiser

prepare_shadow_mask - reads RT output and creates a packed buffer
of the results for use in the denoiser.

********************************************************************/

#define TILE_SIZE_X 8
#define TILE_SIZE_Y 4

[[vk::binding(0)]] cbuffer PassData : register(b0)
{
    int2 BufferDimensions;
}

[[vk::binding(1)]] Texture2DArray<float> t2d_hitMaskResults : register(t0);
[[vk::binding(2)]] RWStructuredBuffer<uint> rwsb_shadowMask : register(u0);

int2 FFX_DNSR_Shadows_GetBufferDimensions()
{
    return BufferDimensions;
}

bool FFX_DNSR_Shadows_HitsLight(uint2 did, uint2 gtid, uint2 gid)
{
    //todo: for now just one light, 1st omni light
    return t2d_hitMaskResults[uint3(did, 1)].x > 0.f;
}

void FFX_DNSR_Shadows_WriteMask(uint offset, uint value)
{
    rwsb_shadowMask[offset] = value;
} 

#include "ffx_denoiser_shadows_prepare.h"

[numthreads(TILE_SIZE_X, TILE_SIZE_Y, 1)]
void main(uint2 gtid : SV_GroupThreadID, uint2 gid : SV_GroupID)
{
    FFX_DNSR_Shadows_PrepareShadowMask(gtid, gid);
}
