#pragma once

#include "Render/RabbitPass.h"

BEGIN_DECLARE_RABBITPASS(TextureDebugPass);

	struct DebugTextureParams
	{
		uint32_t hasMips = false;
		int mipSlice = 0;
		int mipCount = 1;
	
		uint32_t isArray = false;
		int arraySlice = 0;
		int arrayCount = 1;
	
		uint32_t showR = true;
		uint32_t showG = true;
		uint32_t showB = true;
		uint32_t showA = true;
	
		bool is3D;
		float texture3DDepthScale = 0.f;
	};
	
	declareResource(Output, VulkanTexture);
	declareResource(ParamsGPU, VulkanBuffer);
	
	static DebugTextureParams ParamsCPU;

END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(OutlineEntityPass)

	declareResource(Main, VulkanTexture);
	declareResource(HelperBuffer, VulkanBuffer);

END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(CopyToSwapchainPass)
END_DECLARE_RABBITPASS
