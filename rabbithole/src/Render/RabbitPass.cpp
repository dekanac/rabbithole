#include "Render/Camera.h"
#include "Render/RabbitPass.h"
#include "Render/ResourceStateTracking.h"
#include "Render/SuperResolutionManager.h"
#include "Utils/utils.h"

#include "Render/RabbitPasses/AmbientOcclusion.h"
#include "Render/RabbitPasses/GBuffer.h"
#include "Render/RabbitPasses/Lighting.h"
#include "Render/RabbitPasses/Postprocessing.h"
#include "Render/RabbitPasses/Shadows.h"
#include "Render/RabbitPasses/Tools.h"
#include "Render/RabbitPasses/Upscaling.h"
#include "Render/RabbitPasses/Volumetric.h"

void RabbitPass::SetCombinedImageSampler(uint32_t slot, VulkanTexture* texture)
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();
	ResourceStateTrackingManager& rstManager = m_Renderer.GetResourceStateTrackingManager();
	stateManager.UpdateResourceStage(texture);

	texture->SetShouldBeResourceState(ResourceState::GenericRead);
	rstManager.AddResourceForTransition(texture);
	stateManager.SetCombinedImageSampler(slot, texture);
}

void RabbitPass::SetSampledImage(uint32_t slot, VulkanTexture* texture)
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();
	ResourceStateTrackingManager& rstManager = m_Renderer.GetResourceStateTrackingManager();
	stateManager.UpdateResourceStage(texture);

	texture->SetShouldBeResourceState(ResourceState::GenericRead);
	rstManager.AddResourceForTransition(texture);
	stateManager.SetSampledImage(slot, texture);
}

void RabbitPass::SetStorageImage(uint32_t slot, VulkanTexture* texture, uint32_t mipSlice)
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();
	ResourceStateTrackingManager& rstManager = m_Renderer.GetResourceStateTrackingManager();
	stateManager.UpdateResourceStage(texture);

	texture->SetShouldBeResourceState(ResourceState::GeneralCompute);
	rstManager.AddResourceForTransition(texture);
	stateManager.SetStorageImage(slot, texture->GetView(mipSlice));
}

void RabbitPass::SetConstantBuffer(uint32_t slot, VulkanBuffer* buffer)
{
	m_Renderer.GetStateManager().SetConstantBuffer(slot, buffer);
}

void RabbitPass::SetStorageBufferRead(uint32_t slot, VulkanBuffer* buffer)
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();
	ResourceStateTrackingManager& rstManager = m_Renderer.GetResourceStateTrackingManager();
	stateManager.UpdateResourceStage(buffer);

	buffer->SetShouldBeResourceState(ResourceState::BufferRead);
	rstManager.AddResourceForTransition(buffer);
	stateManager.SetStorageBuffer(slot, buffer);
}

void RabbitPass::SetStorageBufferWrite(uint32_t slot, VulkanBuffer* buffer)
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();
	ResourceStateTrackingManager& rstManager = m_Renderer.GetResourceStateTrackingManager();
	stateManager.UpdateResourceStage(buffer);

	buffer->SetShouldBeResourceState(ResourceState::BufferWrite);
	rstManager.AddResourceForTransition(buffer);
	stateManager.SetStorageBuffer(slot, buffer);
}

void RabbitPass::SetStorageBufferReadWrite(uint32_t slot, VulkanBuffer* buffer)
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();
	ResourceStateTrackingManager& rstManager = m_Renderer.GetResourceStateTrackingManager();
	stateManager.UpdateResourceStage(buffer);

	buffer->SetShouldBeResourceState(ResourceState::BufferReadWrite);
	rstManager.AddResourceForTransition(buffer);
	stateManager.SetStorageBuffer(slot, buffer);
}

void RabbitPass::SetSampler(uint32_t slot, VulkanTexture* texture)
{
	m_Renderer.GetStateManager().SetSampler(slot, texture->GetSampler());
}

void RabbitPass::SetRenderTarget(uint32_t slot, VulkanTexture* texture)
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();
	ResourceStateTrackingManager& rstManager = m_Renderer.GetResourceStateTrackingManager();
	stateManager.UpdateResourceStage(texture);

	texture->SetShouldBeResourceState(ResourceState::RenderTarget);
	rstManager.AddResourceForTransition(texture);

	switch (slot)
	{
	case 0:
		stateManager.SetRenderTarget0(texture->GetView());
		break;
	case 1:
		stateManager.SetRenderTarget1(texture->GetView());
		break;
	case 2:
		stateManager.SetRenderTarget2(texture->GetView());
		break;
	case 3:
		stateManager.SetRenderTarget3(texture->GetView());
		break;
	case 4:
		stateManager.SetRenderTarget4(texture->GetView());
		break;
	default:
		ASSERT(false, "Render Target number exceeded!");
		break;
	}
}

void RabbitPass::SetDepthStencil(VulkanTexture* texture)
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();
	ResourceStateTrackingManager& rstManager = m_Renderer.GetResourceStateTrackingManager();
	stateManager.UpdateResourceStage(texture);

	texture->SetShouldBeResourceState(ResourceState::DepthStencilWrite);
	rstManager.AddResourceForTransition(texture);
	stateManager.SetDepthStencil(texture->GetView());
}

//pass scheduler
void RabbitPassManager::SchedulePasses(Renderer& renderer)
{
	AddPass(new Create3DNoiseTexturePass(renderer), true);
	AddPass(new GBufferPass(renderer));
	AddPass(new SSAOPass(renderer));
	AddPass(new SSAOBlurPass(renderer));
	AddPass(new RTShadowsPass(renderer));
	AddPass(new ShadowDenoisePrePass(renderer));
	AddPass(new ShadowDenoiseTileClassificationPass(renderer));
	AddPass(new ShadowDenoiseFilterPass(renderer));
	AddPass(new VolumetricPass(renderer));
	AddPass(new ComputeScatteringPass(renderer));
	AddPass(new SkyboxPass(renderer));
	AddPass(new LightingPass(renderer));
	AddPass(new ApplyVolumetricFogPass(renderer));
	AddPass(new TextureDebugPass(renderer));
	AddPass(new FSR2Pass(renderer));
	AddPass(new BloomCompute(renderer));
	AddPass(new TonemappingPass(renderer));
	AddPass(new CopyToSwapchainPass(renderer));
}

void RabbitPassManager::DeclareResources()
{
	for (auto pass : m_RabbitPassesToExecute)
	{
		pass->DeclareResources();
	}
}

void RabbitPassManager::ExecutePasses(Renderer& renderer)
{
	for (auto pass : m_RabbitPassesToExecute)
	{
		renderer.BeginLabel(pass->GetName());

		pass->Setup();

		pass->Render();

		renderer.RecordGPUTimeStamp(pass->GetName());

		renderer.EndLabel();		
	}
}

void RabbitPassManager::ExecuteOneTimePasses(Renderer& renderer)
{
	for (auto pass : m_RabbitPassesOneTimeExecute)
	{
		renderer.BeginLabel(pass->GetName());

		pass->Setup();

		pass->Render();

		renderer.RecordGPUTimeStamp(pass->GetName());

		renderer.EndLabel();
	}
}

void RabbitPassManager::Destroy()
{

}
