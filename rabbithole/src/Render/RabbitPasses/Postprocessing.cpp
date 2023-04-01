#include "Postprocessing.h"

#include "Render/RabbitPasses/Lighting.h"
#include "Render/RabbitPasses/Upscaling.h"

defineResource(TonemappingPass, Output, VulkanTexture);

defineResource(BloomCompute, Downsampled, VulkanTexture);
defineResource(BloomCompute, BloomParamsGPU, VulkanBuffer);
defineResource(BloomCompute, BloomApplied, VulkanTexture);

BloomCompute::BloomParams BloomCompute::BloomParamsCPU = {};

void BloomCompute::DeclareResources()
{
	BloomApplied = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = { GetUpscaledWidth, GetUpscaledHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Applied Bloom Output"},
			.arraySize = {1},
			.isCube = {false},
			.multisampleType = {MultisampleType::Sample_1},
			.samplerType = {SamplerType::Bilinear},
			.addressMode = {AddressMode::Clamp},
			.mipCount = {1}
		});

	Downsampled = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = { GetUpscaledWidth, GetUpscaledHeight, 1},
			.flags = {TextureFlags::Storage | TextureFlags::Read},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Bloom Downsampled"},
			.arraySize = {1},
			.isCube = {false},
			.multisampleType = {MultisampleType::Sample_1},
			.samplerType = {SamplerType::Trilinear},
			.addressMode = {AddressMode::Clamp},
			.mipCount = {DOWNSAMPLE_COUNT}
		});

	for (uint32_t i = 0; i < DOWNSAMPLE_COUNT; i++)
	{
		m_DownsampledMipChain.push_back(m_Renderer.GetResourceManager().CreateSingleMipFromTexture(m_Renderer.GetVulkanDevice(), Downsampled, i));
	}

	BloomParamsGPU = m_Renderer.GetResourceManager().CreateBuffer(m_Renderer.GetVulkanDevice(), BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(BloomParams)},
			.name = {"Bloom Params"}
		});
}

void BloomCompute::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	if (m_Renderer.IsImguiReady())
	{
		ImGui::Begin("BloomParams");
		ImGui::SliderFloat("Threshold:", &BloomParamsCPU.threshold, 0.0f, 5.f);
		ImGui::SliderFloat("Knee:", &BloomParamsCPU.knee, 0.0f, 5.f);
		ImGui::SliderFloat("Exposure:", &BloomParamsCPU.exposure, 0.0f, 5.f);
		ImGui::End();
	}

	uint32_t width = BloomCompute::Downsampled->GetWidth();
	uint32_t height = BloomCompute::Downsampled->GetHeight();

	BloomParamsGPU->FillBuffer(&BloomParamsCPU);

	//DOWNSAMPLE
	stateManager.SetComputeShader(m_Renderer.GetShader("CS_Downsample"));

	SetCombinedImageSampler(0, FSR2Pass::Output);
	SetStorageImageWrite(1, m_DownsampledMipChain[1]);
	SetConstantBuffer(2, BloomCompute::BloomParamsGPU);

	uint32_t downsampleTick = 1;
	m_Renderer.BindPushConst(downsampleTick);

	constexpr uint32_t threadGroupWorkRegionDim = 8;

	width = width / 2 + 1;
	height = height / 2 + 1;

	uint32_t dispatchX = GetCSDispatchCount(width, threadGroupWorkRegionDim);
	uint32_t dispatchY = GetCSDispatchCount(height, threadGroupWorkRegionDim);

	m_Renderer.Dispatch(dispatchX, dispatchY, 1);

	for (uint32_t i = 2; i < DOWNSAMPLE_COUNT; i++)
	{
		width = width / 2 + 1;
		height = height / 2 + 1;

		SetCombinedImageSampler(0, BloomCompute::m_DownsampledMipChain[i-1]);
		SetStorageImageWrite(1, BloomCompute::m_DownsampledMipChain[i]);
		SetConstantBuffer(2, BloomCompute::BloomParamsGPU);
		m_Renderer.BindPushConst(i);
	
		dispatchX = GetCSDispatchCount(width, threadGroupWorkRegionDim);
		dispatchY = GetCSDispatchCount(height, threadGroupWorkRegionDim);
	
		m_Renderer.Dispatch(dispatchX, dispatchY, 1);
	}
	
	//UPSAMPLE
	stateManager.SetComputeShader(m_Renderer.GetShader("CS_Upsample"));
	
	for (uint32_t i = (DOWNSAMPLE_COUNT - 1); i > 0; i--)
	{
		width *= 2;
		height *= 2;

		SetCombinedImageSampler(0, BloomCompute::m_DownsampledMipChain[i]);
		SetStorageImageReadWrite(1, m_DownsampledMipChain[i-1]);
		SetConstantBuffer(2, BloomCompute::BloomParamsGPU);

		uint32_t isLastPass = i == 1;
		m_Renderer.BindPushConst(isLastPass);
	
		dispatchX = GetCSDispatchCount(width, threadGroupWorkRegionDim);
		dispatchY = GetCSDispatchCount(height, threadGroupWorkRegionDim);
	
		m_Renderer.Dispatch(dispatchX, dispatchY, 1);
	}

	//APPLY BLOOM
	stateManager.SetVertexShader(m_Renderer.GetShader("VS_PassThrough"));
	stateManager.SetPixelShader(m_Renderer.GetShader("FS_ApplyBloom"));
	
	m_Renderer.BindViewport(0, 0, static_cast<float>(GetUpscaledWidth), static_cast<float>(GetUpscaledHeight));
	stateManager.SetRenderPassExtent({ GetUpscaledWidth , GetUpscaledHeight });
	
	SetCombinedImageSampler(0, FSR2Pass::Output);
	SetCombinedImageSampler(1, m_DownsampledMipChain[0]);
	SetConstantBuffer(2, BloomCompute::BloomParamsGPU);
	
	SetRenderTarget(0, BloomCompute::BloomApplied);
	
	m_Renderer.DrawFullScreenQuad();
}

void BloomCompute::Render()
{

}

void TonemappingPass::DeclareResources()
{
	Output = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetUpscaledWidth, GetUpscaledHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Post Upscale PostEffects" },
		});
}

void TonemappingPass::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	m_Renderer.BindViewport(0, 0, static_cast<float>(GetUpscaledWidth), static_cast<float>(GetUpscaledHeight));
	stateManager.SetRenderPassExtent({ GetUpscaledWidth , GetUpscaledHeight });

	stateManager.SetVertexShader(m_Renderer.GetShader("VS_PassThrough"));
	stateManager.SetPixelShader(m_Renderer.GetShader("FS_Tonemap"));

	SetCombinedImageSampler(0, BloomCompute::BloomApplied);

	SetRenderTarget(0, TonemappingPass::Output);
}

void TonemappingPass::Render()
{
	m_Renderer.DrawFullScreenQuad();
}
