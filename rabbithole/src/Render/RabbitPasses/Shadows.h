#pragma once

#include "Render/RabbitPass.h"

BEGIN_DECLARE_RABBITPASS(RTShadowsPass);

	declareResource(ShadowMask, VulkanTexture);
	static uint32_t ShadowResX;
	static uint32_t ShadowResY;

	END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(VolumetricShadowsPass);

	declareResource(VolumetricShadows, VulkanTexture);
	
END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(ShadowDenoisePrePass);

	struct DenoiseBufferDimensions
	{
		uint32_t dimensions[2];
	};

	void PrepareDenoisePass(uint32_t shadowSlice);

	declareResource(BufferDimensions, VulkanBuffer);
	declareResourceArray(ShadowData, VulkanBuffer, MAX_NUM_OF_LIGHTS);

END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(ShadowDenoiseTileClassificationPass);

	struct DenoiseShadowData
	{
		rabbitVec3f		Eye;
		int				FirstFrame;
		int32_t			BufferDimensions[2];
		float			InvBufferDimensions[2];
		rabbitMat4f		ProjectionInverse;
		rabbitMat4f		ReprojectionMatrix;
		rabbitMat4f		ViewProjectionInverse;
	};
	
	uint32_t GetCurrentIDFromFrameIndex(uint32_t id) { return (m_Renderer.GetCurrentFrameIndex() + id) % 2; }
	void ClassifyTiles(uint32_t shadowSlice);
	
	declareResource(LastFrameDepth, VulkanTexture);
	declareResourceArray(Moments0, VulkanTexture, MAX_NUM_OF_LIGHTS);
	declareResourceArray(Moments1, VulkanTexture, MAX_NUM_OF_LIGHTS);
	declareResourceArray(Reprojection0, VulkanTexture, MAX_NUM_OF_LIGHTS);
	declareResourceArray(Reprojection1, VulkanTexture, MAX_NUM_OF_LIGHTS);
	
	declareResourceArray(TileMetadata, VulkanBuffer, MAX_NUM_OF_LIGHTS);
	declareResource(ReprojectionInfo, VulkanBuffer);

END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(ShadowDenoiseFilterPass);

	struct DenoiseShadowFilterData
	{
		rabbitMat4f ProjectionInverse;
		int32_t     BufferDimensions[2];
		float		InvBufferDimensions[2];
		float		DepthSimilaritySigma;
	};
	
	void RenderFilterPass0(uint32_t shadowSlice);
	void RenderFilterPass1(uint32_t shadowSlice);
	void RenderFilterPass2(uint32_t shadowSlice);
	
	declareResource(FilterData, VulkanBuffer);
	declareResource(ShadowMask, VulkanTexture);

END_DECLARE_RABBITPASS
