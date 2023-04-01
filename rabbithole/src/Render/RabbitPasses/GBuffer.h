#pragma once

#include "Render/RabbitPass.h"

BEGIN_DECLARE_RABBITPASS(SkyboxPass)

	declareResource(Main, VulkanTexture);

END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(GBufferPass)

	declareResource(Albedo, VulkanTexture);
	declareResource(Normals, VulkanTexture);
	declareResource(Velocity, VulkanTexture);
	declareResource(Emissive, VulkanTexture);
	declareResource(WorldPosition, VulkanTexture);
    declareResource(Depth, VulkanTexture);

END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(CopyDepthPass)

	declareResource(DepthR32, VulkanTexture);

END_DECLARE_RABBITPASS