#include "common.h"

#include "ResourceStateTracking.h"
#include "Renderer.h"
#include "Resource.h"

void ResourceStateTrackingManager::CommitBarriers(Renderer& renderer)
{
	for (auto resource : m_ResourcesForTransition)
	{
		ResourceState resourceState = resource->GetResourceState();
		ResourceState resourceShouldBe = resource->GetShouldBeResourceState();
		ResourceStage resourcePreviousStage = resource->GetPreviousResourceStage();
		ResourceStage resourceCurrentStage = resource->GetCurrentResourceStage();

		if (!(resourceState == resourceShouldBe))
		{
			if (resource->GetType() == ResourceType::Texture)
			{
				VulkanTexture* textureResource = static_cast<VulkanTexture*>(resource);
				uint32_t mipSlice = textureResource->GetRegion().Subresource.MipSlice;
				uint32_t mipCount = textureResource->GetRegion().Subresource.MipSize;

				renderer.ResourceBarrier(textureResource, resourceState, resourceShouldBe, resourcePreviousStage, resourceCurrentStage, mipSlice,mipCount);
			}
			else if (resource->GetType() == ResourceType::Buffer)
			{
				renderer.ResourceBarrier(static_cast<VulkanBuffer*>(resource), resourceState, resourceShouldBe, resourcePreviousStage, resourceCurrentStage);
			}
		}
	}

	Reset();
}

void ResourceStateTrackingManager::AddResourceForTransition(ManagableResource* resource)
{
	m_ResourcesForTransition.push_back(resource);
}

void ResourceStateTrackingManager::Reset()
{
	m_ResourcesForTransition.clear();
}
