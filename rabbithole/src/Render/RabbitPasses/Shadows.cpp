#include "Shadows.h"

#include "Render/RabbitPasses/GBuffer.h"
#include "Render/RabbitPasses/Lighting.h"

defineResource(RTShadowsPass, ShadowMask, VulkanTexture);
uint32_t RTShadowsPass::ShadowResX = 0;
uint32_t RTShadowsPass::ShadowResY = 0;

defineResource(ShadowDenoisePrePass, BufferDimensions, VulkanBuffer);
defineResourceArray(ShadowDenoisePrePass, ShadowData, VulkanBuffer, MAX_NUM_OF_LIGHTS);

defineResource(ShadowDenoiseTileClassificationPass, LastFrameDepth, VulkanTexture);
defineResourceArray(ShadowDenoiseTileClassificationPass, Moments0, VulkanTexture, MAX_NUM_OF_LIGHTS);
defineResourceArray(ShadowDenoiseTileClassificationPass, Moments1, VulkanTexture, MAX_NUM_OF_LIGHTS);
defineResourceArray(ShadowDenoiseTileClassificationPass, Reprojection0, VulkanTexture, MAX_NUM_OF_LIGHTS);
defineResourceArray(ShadowDenoiseTileClassificationPass, Reprojection1, VulkanTexture, MAX_NUM_OF_LIGHTS);
defineResourceArray(ShadowDenoiseTileClassificationPass, TileMetadata, VulkanBuffer, MAX_NUM_OF_LIGHTS);
defineResource(ShadowDenoiseTileClassificationPass, ReprojectionInfo, VulkanBuffer);

defineResource(ShadowDenoiseFilterPass, FilterData, VulkanBuffer);
defineResource(ShadowDenoiseFilterPass, ShadowMask, VulkanTexture);

void RTShadowsPass::DeclareResources()
{
	const float shadowResolutionMultiplier = 1.f;

	ShadowResX = static_cast<uint32_t>(GetNativeWidth * shadowResolutionMultiplier);
	ShadowResY = static_cast<uint32_t>(GetNativeHeight * shadowResolutionMultiplier);

	ShadowMask = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {ShadowResX, ShadowResY, 1},
			.flags = {TextureFlags::Read | TextureFlags::Storage},
			.format = {Format::R8_UNORM},
			.name = {"Shadow Mask"},
			.arraySize = {MAX_NUM_OF_LIGHTS},
		});
}

void RTShadowsPass::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	stateManager.SetComputeShader(m_Renderer.GetShader("CS_RayTracingShadows"));

	SetStorageBufferRead(0, m_Renderer.vertexBuffer);
	SetStorageBufferRead(1, m_Renderer.trianglesBuffer);
	SetStorageBufferRead(2, m_Renderer.triangleIndxsBuffer);
	SetStorageBufferRead(3, m_Renderer.cfbvhNodesBuffer);
	SetStorageImage(4, GBufferPass::WorldPosition);
	SetStorageImage(5, GBufferPass::Normals);
	SetStorageImage(6, RTShadowsPass::ShadowMask);
	SetConstantBuffer(7, LightingPass::LightParamsGPU);
	SetStorageImage(8, m_Renderer.blueNoise2DTexture);
	SetConstantBuffer(9, m_Renderer.GetMainConstBuffer());
}

void RTShadowsPass::Render()
{
	static const int threadGroupWorkRegionDim = 8;

	int texWidth = RTShadowsPass::ShadowMask->GetWidth();
	int texHeight = RTShadowsPass::ShadowMask->GetHeight();

	int dispatchX = GetCSDispatchCount(texWidth, threadGroupWorkRegionDim);
	int dispatchY = GetCSDispatchCount(texHeight, threadGroupWorkRegionDim);

	m_Renderer.Dispatch(dispatchX, dispatchY, numOfLights);
}

void ShadowDenoisePrePass::DeclareResources()
{
	const uint32_t tileW = GetCSDispatchCount(RTShadowsPass::ShadowResX, 8);
	const uint32_t tileH = GetCSDispatchCount(RTShadowsPass::ShadowResY, 4);

	const uint32_t tileSize = tileH * tileW;

	BufferDimensions = m_Renderer.GetResourceManager().CreateBuffer(m_Renderer.GetVulkanDevice(), BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(DenoiseBufferDimensions)},
			.name = {"Denoise Dimensions buffer"}
		});

	for (uint32_t i = 0; i < MAX_NUM_OF_LIGHTS; i++)
	{
		ShadowData[i] = m_Renderer.GetResourceManager().CreateBuffer(m_Renderer.GetVulkanDevice(), BufferCreateInfo{
				.flags = {BufferUsageFlags::StorageBuffer},
				.memoryAccess = {MemoryAccess::GPU},
				.size = {tileSize * static_cast<uint32_t>(sizeof(uint32_t))},
				.name = {std::format("Denoise Shadow Mask Buffer {} slice", i)}
			});
	}
}

void ShadowDenoisePrePass::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	stateManager.SetComputeShader(m_Renderer.GetShader("CS_PrepareShadowMask"));

	DenoiseBufferDimensions bufferDim{};
	bufferDim.dimensions[0] = RTShadowsPass::ShadowResX;
	bufferDim.dimensions[1] = RTShadowsPass::ShadowResY;

	ShadowDenoisePrePass::BufferDimensions->FillBuffer(&bufferDim);
}

void ShadowDenoisePrePass::Render()
{
	for (int i = 0; i < MAX_NUM_OF_LIGHTS; i++)
		PrepareDenoisePass(i);
}

void ShadowDenoisePrePass::PrepareDenoisePass(uint32_t shadowSlice)
{
	m_Renderer.BindPushConst(shadowSlice);

	SetConstantBuffer(0, ShadowDenoisePrePass::BufferDimensions);
	SetSampledImage(1, RTShadowsPass::ShadowMask);
	SetStorageBufferWrite(2, ShadowDenoisePrePass::ShadowData[shadowSlice]);

	constexpr uint32_t threadGroupWorkRegionDimX = 8;
	constexpr uint32_t threadGroupWorkRegionDimY = 4;

	int dispatchX = GetCSDispatchCount(RTShadowsPass::ShadowResX, threadGroupWorkRegionDimX * 4);
	int dispatchY = GetCSDispatchCount(RTShadowsPass::ShadowResY, threadGroupWorkRegionDimY * 4);

	m_Renderer.Dispatch(dispatchX, dispatchY, 1);
}

void ShadowDenoiseTileClassificationPass::DeclareResources()
{
	const uint32_t tileW = GetCSDispatchCount(RTShadowsPass::ShadowResX, 8);
	const uint32_t tileH = GetCSDispatchCount(RTShadowsPass::ShadowResY, 4);

	const uint32_t tileSize = tileH * tileW;

    LastFrameDepth = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
            .dimensions = {RTShadowsPass::ShadowResX, RTShadowsPass::ShadowResY, 1},
            .flags = {TextureFlags::Read | TextureFlags::TransferDst},
            .format = {Format::R32_SFLOAT},
            .name = {"Last Frame DepthR32"}
        });

	ReprojectionInfo = m_Renderer.GetResourceManager().CreateBuffer(m_Renderer.GetVulkanDevice(), BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(DenoiseShadowData)},
			.name = {"Denoise Shadow Data Buffer"}
		});

	for (uint32_t i = 0; i < MAX_NUM_OF_LIGHTS; i++)
	{

		TileMetadata[i] = m_Renderer.GetResourceManager().CreateBuffer(m_Renderer.GetVulkanDevice(), BufferCreateInfo{
				.flags = {BufferUsageFlags::StorageBuffer},
				.memoryAccess = {MemoryAccess::GPU},
				.size = {tileSize * static_cast<uint32_t>(sizeof(uint32_t))},
				.name = {std::format("Denoise Tile Metadata Buffer {} slice", i)}
			});

		//classify
		Moments0[i] = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
				.dimensions = {RTShadowsPass::ShadowResX, RTShadowsPass::ShadowResY, 1},
				.flags = {TextureFlags::Read | TextureFlags::Storage},
				.format = {Format::R11G11B10_FLOAT},
				.name = {std::format("Denoise Moments Buffer0 {} slice", i)}
			});

		Moments1[i] = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
				.dimensions = {RTShadowsPass::ShadowResX, RTShadowsPass::ShadowResY, 1},
				.flags = {TextureFlags::Read | TextureFlags::Storage},
				.format = {Format::R11G11B10_FLOAT},
				.name = {std::format("Denoise Moments Buffer1 {} slice", i)}
			});

		Reprojection0[i] = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
				.dimensions = {RTShadowsPass::ShadowResX, RTShadowsPass::ShadowResY, 1},
				.flags = {TextureFlags::Read | TextureFlags::Storage},
				.format = {Format::R16G16_FLOAT},
				.name = {std::format("Denoise Reprojection Buffer0 {} slice", i)}
			});

		Reprojection1[i] = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
				.dimensions = {RTShadowsPass::ShadowResX, RTShadowsPass::ShadowResY, 1},
				.flags = {TextureFlags::Read | TextureFlags::Storage},
				.format = {Format::R16G16_FLOAT},
				.name = {std::format("Denoise Reprojection Buffer1 {} slice", i)}
			});
	}
}

void ShadowDenoiseTileClassificationPass::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	stateManager.SetComputeShader(m_Renderer.GetShader("CS_TileClassification"));

	CameraState& cameraState = m_Renderer.GetCameraState();

	DenoiseShadowData shadowData{};
	shadowData.Eye = cameraState.CameraPosition;
	shadowData.FirstFrame = (int)(m_Renderer.GetCurrentFrameIndex() == 0);
	shadowData.BufferDimensions[0] = RTShadowsPass::ShadowResX;
	shadowData.BufferDimensions[1] = RTShadowsPass::ShadowResY;
	shadowData.InvBufferDimensions[0] = 1.f / float(RTShadowsPass::ShadowResX);
	shadowData.InvBufferDimensions[1] = 1.f / float(RTShadowsPass::ShadowResY);
	shadowData.ProjectionInverse = cameraState.ProjectionInverseMatrix;
	shadowData.ViewProjectionInverse = cameraState.ViewProjInverseMatrix;
	shadowData.ReprojectionMatrix = cameraState.ProjectionMatrix * (cameraState.PrevViewMatrix * cameraState.ViewProjInverseMatrix);
	ReprojectionInfo->FillBuffer(&shadowData);
}

void ShadowDenoiseTileClassificationPass::Render()
{
	for (int i = 0; i < MAX_NUM_OF_LIGHTS; i++)
		ClassifyTiles(i);
}

void ShadowDenoiseTileClassificationPass::ClassifyTiles(uint32_t shadowSlice)
{
	SetConstantBuffer(0, ShadowDenoiseTileClassificationPass::ReprojectionInfo);
	SetSampledImage(1, CopyDepthPass::DepthR32);
	SetSampledImage(2, GBufferPass::Velocity);
	SetSampledImage(3, GBufferPass::Normals);
	SetSampledImage(4, ShadowDenoiseTileClassificationPass::Reprojection1[shadowSlice]);
	SetSampledImage(5, ShadowDenoiseTileClassificationPass::LastFrameDepth);
	SetStorageBufferRead(6, ShadowDenoisePrePass::ShadowData[shadowSlice]);
	SetStorageBufferWrite(7, ShadowDenoiseTileClassificationPass::TileMetadata[shadowSlice]);
	SetStorageImage(8, ShadowDenoiseTileClassificationPass::Reprojection0[shadowSlice]);
	SetSampledImage(9, GetCurrentIDFromFrameIndex(0) ? ShadowDenoiseTileClassificationPass::Moments0[shadowSlice] : ShadowDenoiseTileClassificationPass::Moments1[shadowSlice]);
	SetStorageImage(10, GetCurrentIDFromFrameIndex(1) ? ShadowDenoiseTileClassificationPass::Moments0[shadowSlice] : ShadowDenoiseTileClassificationPass::Moments1[shadowSlice]);
	SetSampler(11, ShadowDenoiseFilterPass::ShadowMask);

	constexpr uint32_t threadGroupWorkRegionDim = 8;

	int dispatchX = GetCSDispatchCount(RTShadowsPass::ShadowResX, threadGroupWorkRegionDim);
	int dispatchY = GetCSDispatchCount(RTShadowsPass::ShadowResY, threadGroupWorkRegionDim);

	m_Renderer.Dispatch(dispatchX, dispatchY, 1);
}

void ShadowDenoiseFilterPass::DeclareResources()
{
	FilterData = m_Renderer.GetResourceManager().CreateBuffer(m_Renderer.GetVulkanDevice(), BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(DenoiseShadowFilterData)},
			.name = {"Denoise Shadow Filter Data Buffer"}
		});

	ShadowMask = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {RTShadowsPass::ShadowResX, RTShadowsPass::ShadowResY, 1},
			.flags = {TextureFlags::Read | TextureFlags::Storage},
			.format = {Format::R16G16B16A16_UNORM},
			.name = {"Shadow Mask Denoised"},
			.arraySize = {MAX_NUM_OF_LIGHTS},
			.isCube = { false },
			.multisampleType = {MultisampleType::Sample_1},
			.samplerType = { SamplerType::Trilinear },
			.addressMode = { AddressMode::Clamp }
		});
}

void ShadowDenoiseFilterPass::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();
	CameraState& cameraState = m_Renderer.GetCameraState();

	DenoiseShadowFilterData shadowFilterData{};
	shadowFilterData.ProjectionInverse = cameraState.ProjectionInverseMatrix;
	shadowFilterData.DepthSimilaritySigma = 1.f;
	shadowFilterData.BufferDimensions[0] = static_cast<int>(GetNativeWidth);
	shadowFilterData.BufferDimensions[1] = static_cast<int>(GetNativeHeight);
	shadowFilterData.InvBufferDimensions[0] = 1.f / float(GetNativeWidth);
	shadowFilterData.InvBufferDimensions[1] = 1.f / float(GetNativeHeight);

	ShadowDenoiseFilterPass::FilterData->FillBuffer(&shadowFilterData);

	m_Renderer.CopyImage(GBufferPass::Depth, ShadowDenoiseTileClassificationPass::LastFrameDepth);
}

void ShadowDenoiseFilterPass::Render()
{
	for (uint32_t i = 0; i < MAX_NUM_OF_LIGHTS; i++)
	{
		RenderFilterPass0(i);
		RenderFilterPass1(i);
		RenderFilterPass2(i);
	}
}

void ShadowDenoiseFilterPass::RenderFilterPass0(uint32_t shadowSlice)
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	stateManager.SetComputeShader(m_Renderer.GetShader("CS_FilterSoftShadowsPass0"), "Pass0");

	SetConstantBuffer(0, ShadowDenoiseFilterPass::FilterData);
	SetSampledImage(1, CopyDepthPass::DepthR32);
	SetSampledImage(2, GBufferPass::Normals);
	SetStorageBufferRead(3, ShadowDenoiseTileClassificationPass::TileMetadata[shadowSlice]);
	SetSampledImage(4, ShadowDenoiseTileClassificationPass::Reprojection0[shadowSlice]);
	SetStorageImage(5, ShadowDenoiseTileClassificationPass::Reprojection1[shadowSlice]);

	constexpr uint32_t threadGroupWorkRegionDim = 8;

	uint32_t dispatchX = GetCSDispatchCount(RTShadowsPass::ShadowResX, threadGroupWorkRegionDim);
	uint32_t dispatchY = GetCSDispatchCount(RTShadowsPass::ShadowResY, threadGroupWorkRegionDim);

	m_Renderer.Dispatch(dispatchX, dispatchY, 1);
}

void ShadowDenoiseFilterPass::RenderFilterPass1(uint32_t shadowSlice)
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	stateManager.SetComputeShader(m_Renderer.GetShader("CS_FilterSoftShadowsPass1"), "Pass1");

	SetConstantBuffer(0, ShadowDenoiseFilterPass::FilterData);
	SetSampledImage(1, CopyDepthPass::DepthR32);
	SetSampledImage(2, GBufferPass::Normals);
	SetStorageBufferRead(3, ShadowDenoiseTileClassificationPass::TileMetadata[shadowSlice]);
	SetSampledImage(4, ShadowDenoiseTileClassificationPass::Reprojection1[shadowSlice]);
	SetStorageImage(5, ShadowDenoiseTileClassificationPass::Reprojection0[shadowSlice]);

	constexpr uint32_t threadGroupWorkRegionDim = 8;

	uint32_t dispatchX = GetCSDispatchCount(RTShadowsPass::ShadowResX, threadGroupWorkRegionDim);
	uint32_t dispatchY = GetCSDispatchCount(RTShadowsPass::ShadowResY, threadGroupWorkRegionDim);

	m_Renderer.Dispatch(dispatchX, dispatchY, 1);
}

void ShadowDenoiseFilterPass::RenderFilterPass2(uint32_t shadowSlice)
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	stateManager.SetComputeShader(m_Renderer.GetShader("CS_FilterSoftShadowsPass2"), "Pass2");

	m_Renderer.BindPushConst(shadowSlice);

	SetConstantBuffer(0, ShadowDenoiseFilterPass::FilterData);
	SetSampledImage(1, CopyDepthPass::DepthR32);
	SetSampledImage(2, GBufferPass::Normals);
	SetStorageBufferRead(3, ShadowDenoiseTileClassificationPass::TileMetadata[shadowSlice]);
	SetSampledImage(4, ShadowDenoiseTileClassificationPass::Reprojection0[shadowSlice]);
	SetStorageImage(6, ShadowDenoiseFilterPass::ShadowMask);

	constexpr uint32_t threadGroupWorkRegionDim = 8;

	uint32_t dispatchX = GetCSDispatchCount(RTShadowsPass::ShadowResX, threadGroupWorkRegionDim);
	uint32_t dispatchY = GetCSDispatchCount(RTShadowsPass::ShadowResY, threadGroupWorkRegionDim);

	m_Renderer.Dispatch(dispatchX, dispatchY, 1);
}