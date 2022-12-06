#pragma once

#include "Render/RabbitPass.h"

#define DOWNSAMPLE_COUNT (6)

BEGIN_DECLARE_RABBITPASS(BloomCompute)

	struct BloomParams
	{
		float		threshold = 1.f;
		float		knee = 1.f;
		float		exposure = 1.f;
	};
	
	declareResource(Downsampled, VulkanTexture);
	declareResource(BloomParamsGPU, VulkanBuffer);
	declareResource(BloomApplied, VulkanTexture);
	
	static BloomParams BloomParamsCPU;

private:
	std::vector<VulkanTexture*> m_DownsampledMipChain;

END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(TonemappingPass);

	declareResource(Output, VulkanTexture);

END_DECLARE_RABBITPASS