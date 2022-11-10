#pragma once

#include "Render/RabbitPass.h"

BEGIN_DECLARE_RABBITPASS(SkyboxPass)

	declareResource(Main, VulkanTexture);
	void CopyDepth();

END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(GBufferPass)

	declareResource(Albedo, VulkanTexture);
	declareResource(Normals, VulkanTexture);
	declareResource(Velocity, VulkanTexture);
	declareResource(WorldPosition, VulkanTexture);
    declareResource(Depth, VulkanTexture);
    declareResource(DepthR32, VulkanTexture);

END_DECLARE_RABBITPASS