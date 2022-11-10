#pragma once

#include "Render/RabbitPass.h"

BEGIN_DECLARE_RABBITPASS(SSAOPass);

	bool ComputeSSAO = true;

	float lerp(float a, float b, float f)
	{
		return a + f * (b - a);
	}
	
	struct SSAOSamples
	{
		rabbitVec3f samples[64];
	};
	
	struct SSAOParams
	{
		float	radius;
		float	bias;
		float	resWidth;
		float	resHeight;
		float	power;
		int		kernelSize;
		bool	ssaoOn;
	};

	declareResource(Output, VulkanTexture);
	declareResource(Noise, VulkanTexture);
	declareResource(Samples, VulkanBuffer);
	declareResource(ParamsGPU, VulkanBuffer);
	static SSAOParams ParamsCPU;

END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(SSAOBlurPass);

	declareResource(BluredOutput, VulkanTexture);

END_DECLARE_RABBITPASS
