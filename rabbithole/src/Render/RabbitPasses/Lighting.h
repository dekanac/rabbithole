#pragma once

#include "Render/RabbitPass.h"

BEGIN_DECLARE_RABBITPASS(LightingPass)

	declareResource(MainLighting, VulkanTexture);
	declareResource(LightParamsGPU, VulkanBuffer);

END_DECLARE_RABBITPASS