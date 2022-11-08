#include "Postprocessing.h"

#include "Render/RabbitPasses/Lighting.h"
#include "Render/RabbitPasses/Upscaling.h"

defineResource(TonemappingPass, Output, VulkanTexture);

defineResource(BloomCompute, Downsampled, VulkanTexture);
defineResource(BloomCompute, BloomParamsGPU, VulkanBuffer);
BloomCompute::BloomParams BloomCompute::BloomParamsCPU[DOWNSAMPLE_COUNT] = {};

void BloomCompute::Downsample(VulkanTexture* input, VulkanTexture* output, Extent2D resolution, uint32_t mipLevel)
{

}

void BloomCompute::DeclareResources()
{
	Downsampled = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = { GetNativeWidth / 2, GetNativeHeight / 2, 1},
			.flags = {TextureFlags::Storage | TextureFlags::Read},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Bloom Output"},
			.arraySize = {1},
			.isCube = {false},
			.multisampleType = {MultisampleType::Sample_1},
			.samplerType = {SamplerType::Bilinear},
			.addressMode = {AddressMode::Border},
			.mipCount = {5}
		});

	BloomParamsGPU = m_Renderer.GetResourceManager().CreateBuffer(m_Renderer.GetVulkanDevice(), BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(BloomParams) * DOWNSAMPLE_COUNT},
			.name = {"Bloom Params"}
		});
}

void BloomCompute::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	stateManager.SetComputeShader(m_Renderer.GetShader("CS_Downsample"));

	if (m_Renderer.imguiReady)
	{
		ImGui::Begin("BloomParams");
		ImGui::SliderFloat("Threshold:", &BloomParamsCPU[0].threshold, 0.0f, 1.f);
		ImGui::End();
	}

	uint32_t width = LightingPass::MainLighting->GetWidth();
	uint32_t height = LightingPass::MainLighting->GetHeight();

	width /= 2;
	height /= 2;

	BloomParamsCPU[0].mipLevel = 0;
	BloomParamsCPU[0].texelSize[0] = width;
	BloomParamsCPU[0].texelSize[1] = height;

	BloomParamsCPU[1].mipLevel = 0;
	BloomParamsCPU[1].texelSize[0] = width/2;
	BloomParamsCPU[1].texelSize[1] = height/2;

	BloomParamsCPU[2].mipLevel = 1;
	BloomParamsCPU[2].texelSize[0] = width / 4;
	BloomParamsCPU[2].texelSize[1] = height / 4;

	BloomParamsCPU[3].mipLevel = 2;
	BloomParamsCPU[3].texelSize[0] = width / 8;
	BloomParamsCPU[3].texelSize[1] = height / 8;

	BloomParamsCPU[4].mipLevel = 3;
	BloomParamsCPU[4].texelSize[0] = width / 16;
	BloomParamsCPU[4].texelSize[1] = height / 16;

	BloomParamsGPU->FillBuffer(&BloomParamsCPU);

	SetStorageImage(0, LightingPass::MainLighting);
	SetStorageImage(1, BloomCompute::Downsampled, 0);
	SetConstantBuffer(2, BloomCompute::BloomParamsGPU);

	m_Renderer.BindPushConst(0);

	constexpr uint32_t threadGroupWorkRegionDim = 8;

	uint32_t dispatchX = GetCSDispatchCount(width, threadGroupWorkRegionDim);
	uint32_t dispatchY = GetCSDispatchCount(height, threadGroupWorkRegionDim);

	m_Renderer.Dispatch(dispatchX, dispatchY, 1);

	for (uint32_t i = 1; i < DOWNSAMPLE_COUNT; i++)
	{
		width /= 2;
		height /= 2;
	
		SetStorageImage(0, BloomCompute::Downsampled);
		SetStorageImage(1, BloomCompute::Downsampled, i);
		SetConstantBuffer(2, BloomCompute::BloomParamsGPU);
		m_Renderer.BindPushConst(i);
	
		dispatchX = GetCSDispatchCount(width, threadGroupWorkRegionDim);
		dispatchY = GetCSDispatchCount(height, threadGroupWorkRegionDim);
	
		m_Renderer.Dispatch(dispatchX, dispatchY, 1);
	}
	//
	//stateManager.SetComputeShader(m_Renderer.GetShader("CS_Upsample"));
	//
	//for (uint32_t i = (DOWNSAMPLE_COUNT - 1); i > 0; i--)
	//{
	//	width *= 2;
	//	height *= 2;
	//
	//	SetCombinedImageSampler(0, BloomCompute::Downsampled);
	//	SetStorageImage(1, BloomCompute::Downsampled, i-1);
	//	SetConstantBuffer(2, BloomCompute::BloomParamsGPU);
	//	m_Renderer.BindPushConst((uint32_t)i);
	//
	//	dispatchX = GetCSDispatchCount(width, threadGroupWorkRegionDim);
	//	dispatchY = GetCSDispatchCount(height, threadGroupWorkRegionDim);
	//
	//	m_Renderer.Dispatch(dispatchX, dispatchY, 1);
	//}


	//for (uint32_t i = 0; i < 4; i++)
	//{
	//	SetStorageImage(0, BloomCompute::Downsampled, i);
	//	SetStorageImage(1, BloomCompute::Downsampled, i + 1);
	//	SetConstantBuffer(2, BloomCompute::BloomParamsGPU);
	//
	//	width /= 2;
	//	height /= 2;
	//	uint32_t dispatchX = GetCSDispatchCount(width, threadGroupWorkRegionDim);
	//	uint32_t dispatchY = GetCSDispatchCount(height, threadGroupWorkRegionDim);
	//	m_Renderer.Dispatch(dispatchX, dispatchY, 1);
	//}
	//{
	//	stateManager.SetComputeShader(m_Renderer.GetShader("CS_Upsample"));
	//
	//	SetStorageImage(0, BloomCompute::Downsampled, 4);
	//	SetStorageImage(1, BloomCompute::Downsampled, 3);
	//	SetConstantBuffer(2, BloomCompute::BloomParamsGPU);
	//
	//	constexpr uint32_t threadGroupWorkRegionDim = 8;
	//
	//	uint32_t dispatchX = GetCSDispatchCount(width, threadGroupWorkRegionDim);
	//	uint32_t dispatchY = GetCSDispatchCount(height, threadGroupWorkRegionDim);
	//
	//	m_Renderer.Dispatch(dispatchX, dispatchY, 1);
	//}
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

	stateManager.SetFramebufferExtent(Extent2D{ GetUpscaledWidth , GetUpscaledHeight });
	m_Renderer.BindViewport(0, 0, static_cast<float>(GetUpscaledWidth), static_cast<float>(GetUpscaledHeight));

	stateManager.SetVertexShader(m_Renderer.GetShader("VS_PassThrough"));
	stateManager.SetPixelShader(m_Renderer.GetShader("FS_Tonemap"));

	SetCombinedImageSampler(0, FSR2Pass::Output);

	SetRenderTarget(0, TonemappingPass::Output);
}

void TonemappingPass::Render()
{
	m_Renderer.DrawFullScreenQuad();
}
