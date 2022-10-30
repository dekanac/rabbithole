#pragma once

#include "Render/RabbitPass.h"

BEGIN_DECLARE_RABBITPASS(VolumetricPass);

	struct VolumetricFogParams
	{
		uint32_t	isEnabled = true;
		float		fogAmount = 0.006f;
		float		depthScale_debug = 2.f;
		float		fogStartDistance = 0.1f;
		float		fogDistance = 64.f;
	};

	declareResource(MediaDensity, VulkanTexture);
	declareResource(ParamsGPU, VulkanBuffer);

	static VolumetricFogParams ParamsCPU;

END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(ComputeScatteringPass);

	declareResource(LightScattering, VulkanTexture);

END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(Create3DNoiseTexturePass);
END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(ApplyVolumetricFogPass);

	declareResource(Output, VulkanTexture);

END_DECLARE_RABBITPASS
