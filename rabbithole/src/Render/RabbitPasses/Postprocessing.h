#pragma once

#include "Render/RabbitPass.h"

#define DOWNSAMPLE_COUNT (5)

BEGIN_DECLARE_RABBITPASS(BloomCompute)

	struct BloomParams
	{
		float		threshold = 1.f;
		uint32_t	texelSize[2] = { 0, 0 };
		uint32_t	mipLevel = 0;
	};
	
	declareResource(Downsampled, VulkanTexture);
	declareResource(BloomParamsGPU, VulkanBuffer);
	
	static BloomParams BloomParamsCPU[DOWNSAMPLE_COUNT];
	
	void Downsample(VulkanTexture* input, VulkanTexture* output, Extent2D resolution, uint32_t mipLevel);

END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(TonemappingPass);

	declareResource(Output, VulkanTexture);

END_DECLARE_RABBITPASS