#pragma once

#include "Render/RabbitPass.h"

constexpr size_t g_VolumetricTexX = 160;
constexpr size_t g_VolumetricTexY = 90;
#if defined(VULKAN_HWRT)
constexpr size_t g_VolumetricTexZ = 512;
#else
constexpr size_t g_VolumetricTexZ = 128;
#endif

BEGIN_DECLARE_RABBITPASS(VolumetricPass);

	struct VolumetricFogParams
	{
		uint32_t	isEnabled = true;
		float		fogAmount = 0.006f;
		float		depthScale_debug = 2.f;
		float		fogStartDistance = 0.1f;
		float		fogDistance = 128.f;
		float		fogDepthExponent = 1.7f;
		uint32_t	volumetricTexWidth = 160;
		uint32_t	volumetricTexHeight = 90;
		uint32_t	volumetricTexDepth = 384;
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
