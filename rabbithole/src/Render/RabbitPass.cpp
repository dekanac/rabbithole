#include "Render/Camera.h"
#include "Render/RabbitPass.h"
#include "Render/ResourceStateTracking.h"
#include "Render/SuperResolutionManager.h"
#include "Utils/utils.h"

void RabbitPass::SetCombinedImageSampler(uint32_t slot, VulkanTexture* texture, uint32_t mipSlice)
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();
	ResourceStateTrackingManager& rstManager = m_Renderer.GetResourceStateTrackingManager();
	stateManager.UpdateResourceStage(texture);

	texture->SetShouldBeResourceState(ResourceState::GenericRead);
	rstManager.AddResourceForTransition(texture);
	stateManager.SetCombinedImageSampler(slot, texture, mipSlice);
}

void RabbitPass::SetSampledImage(uint32_t slot, VulkanTexture* texture, uint32_t mipSlice)
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();
	ResourceStateTrackingManager& rstManager = m_Renderer.GetResourceStateTrackingManager();
	stateManager.UpdateResourceStage(texture);

	texture->SetShouldBeResourceState(ResourceState::GenericRead);
	rstManager.AddResourceForTransition(texture);
	stateManager.SetSampledImage(slot, texture->GetView(mipSlice));
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