#include "Tools.h"

#include "Render/RabbitPasses/Lighting.h"
#include "Render/RabbitPasses/Postprocessing.h"

defineResource(TextureDebugPass, Output, VulkanTexture);
defineResource(TextureDebugPass, ParamsGPU, VulkanBuffer);
TextureDebugPass::DebugTextureParams TextureDebugPass::ParamsCPU = {};

defineResource(OutlineEntityPass, Main, VulkanTexture);
defineResource(OutlineEntityPass, HelperBuffer, VulkanBuffer);

void CopyToSwapchainPass::DeclareResources()
{

}

void CopyToSwapchainPass::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	uint32_t renderWindowWidth = Window::instance().GetExtent().width;
	uint32_t renderWindowHeight = Window::instance().GetExtent().height;

	m_Renderer.BindViewport(0, 0, static_cast<float>(renderWindowWidth), static_cast<float>(renderWindowHeight));
	stateManager.SetRenderPassExtent({ renderWindowWidth , renderWindowHeight });

	stateManager.SetVertexShader(m_Renderer.GetShader("VS_PassThrough"));
	stateManager.SetPixelShader(m_Renderer.GetShader("FS_PassThrough"));

	SetCombinedImageSampler(0, TonemappingPass::Output);

	stateManager.SetRenderTarget(0, m_Renderer.GetSwapchainImage());

	auto pipelineInfo = stateManager.GetPipelineInfo();
	pipelineInfo->SetDepthTestEnabled(false);

	auto renderPassInfo = stateManager.GetRenderPassInfo();
	renderPassInfo->InitialRenderTargetState = ResourceState::None;
	renderPassInfo->FinalRenderTargetState = ResourceState::Present;
	renderPassInfo->InitialDepthStencilState = ResourceState::DepthStencilWrite;
	renderPassInfo->FinalDepthStencilState = ResourceState::None;
}

void CopyToSwapchainPass::Render()
{
	m_Renderer.CopyToSwapChain();
}

void OutlineEntityPass::DeclareResources()
{
	Main = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::TransferSrc},
			.format = {Format::R32_UINT},
			.name = {"Entity Helper"}
		});

	HelperBuffer = m_Renderer.GetResourceManager().CreateBuffer(m_Renderer.GetVulkanDevice(), BufferCreateInfo{
			.flags = {BufferUsageFlags::StorageBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {GetNativeWidth * GetNativeHeight * 4},
			.name = {"Entity Helper"}
		});
}

void OutlineEntityPass::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	stateManager.SetVertexShader(m_Renderer.GetShader("VS_PassThrough"));
	stateManager.SetPixelShader(m_Renderer.GetShader("FS_OutlineEntity"));

	stateManager.ShouldCleanColor(false);

	m_Renderer.UpdateEntityPickId();

	auto pipelineInfo = stateManager.GetPipelineInfo();
	pipelineInfo->SetAttachmentCount(1);
	pipelineInfo->SetColorWriteMask(0, ColorWriteMaskFlags::RGBA);

	auto renderPassInfo = stateManager.GetRenderPassInfo();
	renderPassInfo->InitialRenderTargetState = ResourceState::None;
	renderPassInfo->FinalRenderTargetState = ResourceState::RenderTarget;

	SetCombinedImageSampler(0, m_Renderer.entityHelper);
	SetRenderTarget(0, LightingPass::MainLighting);
}

void OutlineEntityPass::Render()
{
	m_Renderer.DrawFullScreenQuad();
}

void TextureDebugPass::DeclareResources()
{
	Output = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
		.dimensions = {GetNativeWidth, GetNativeHeight, 1},
		.flags = {TextureFlags::RenderTarget | TextureFlags::Read},
		.format = {Format::R16G16B16A16_FLOAT},
		.name = {"Debug Texture"}
		});

	ParamsGPU = m_Renderer.GetResourceManager().CreateBuffer(m_Renderer.GetVulkanDevice(), BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(DebugTextureParams)},
			.name = {"Debug Texture Params"}
		});
}

void TextureDebugPass::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	stateManager.SetVertexShader(m_Renderer.GetShader("VS_PassThrough"));
	stateManager.SetPixelShader(m_Renderer.GetShader("FS_TextureDebug"));

	SetConstantBuffer(0, TextureDebugPass::ParamsGPU);

	auto textureToBind = m_Renderer.g_DefaultWhiteTexture;
	auto texture3DToBind = m_Renderer.g_Default3DTexture;
	auto textureArrayToBind = m_Renderer.g_DefaultArrayTexture;

	VulkanTexture* selectedTexture = m_Renderer.GetTextureWithID(m_Renderer.currentTextureSelectedID);

	if (selectedTexture)
	{
		auto& debugParams = TextureDebugPass::ParamsCPU;
		if (debugParams.isArray)
		{
			textureArrayToBind = selectedTexture;
		}
		else if (debugParams.is3D)
		{
			texture3DToBind = selectedTexture;
		}
		else
		{
			textureToBind = selectedTexture;
		}
	}

	SetCombinedImageSampler(1, textureToBind);
	SetCombinedImageSampler(2, textureArrayToBind);
	SetCombinedImageSampler(3, texture3DToBind);

	TextureDebugPass::ParamsGPU->FillBuffer(&TextureDebugPass::ParamsCPU);

	SetRenderTarget(0, TextureDebugPass::Output);

}

void TextureDebugPass::Render()
{
	m_Renderer.DrawFullScreenQuad();

	m_Renderer.ResourceBarrier(TextureDebugPass::Output, ResourceState::RenderTarget, ResourceState::GenericRead, ResourceStage::Graphics, ResourceStage::Graphics);
}