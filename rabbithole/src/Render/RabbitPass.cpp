#include "Render/Camera.h"
#include "Render/RabbitPass.h"
#include "Render/ResourceStateTracking.h"
#include "Render/SuperResolutionManager.h"
#include "Render/Raytracing.h"
#include "Utils/utils.h"

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
	stateManager.SetSampledImage(slot, texture->GetView());
}

void RabbitPass::SetStorageImageRead(uint32_t slot, VulkanTexture* texture)
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();
	ResourceStateTrackingManager& rstManager = m_Renderer.GetResourceStateTrackingManager();
	stateManager.UpdateResourceStage(texture);

	texture->SetShouldBeResourceState(ResourceState::GeneralComputeRead);
	rstManager.AddResourceForTransition(texture);
	stateManager.SetStorageImage(slot, texture->GetView());
}

void RabbitPass::SetStorageImageWrite(uint32_t slot, VulkanTexture* texture)
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();
	ResourceStateTrackingManager& rstManager = m_Renderer.GetResourceStateTrackingManager();
	stateManager.UpdateResourceStage(texture);

	texture->SetShouldBeResourceState(ResourceState::GeneralComputeWrite);
	rstManager.AddResourceForTransition(texture);
	stateManager.SetStorageImage(slot, texture->GetView());
}

void RabbitPass::SetStorageImageReadWrite(uint32_t slot, VulkanTexture* texture)
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();
	ResourceStateTrackingManager& rstManager = m_Renderer.GetResourceStateTrackingManager();
	stateManager.UpdateResourceStage(texture);

	texture->SetShouldBeResourceState(ResourceState::GeneralComputeReadWrite);
	rstManager.AddResourceForTransition(texture);
	stateManager.SetStorageImage(slot, texture->GetView());
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
	stateManager.SetRenderTarget(slot, texture->GetView());
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

#if defined(VULKAN_HWRT)
void RabbitPass::SetAccelerationStructure(uint32_t slot, RayTracing::AccelerationStructure* as)
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();
	stateManager.SetAccelerationStructure(slot, as);
}
#endif
