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

				renderer.ResourceBarrier(
					textureResource, 
					resourceState, 
					resourceShouldBe, 
					resourcePreviousStage, 
					resourceCurrentStage, 
					textureResource->GetRegion().Subresource.MipSlice,
					textureResource->GetRegion().Subresource.MipSize);
			}
			else if (resource->GetType() == ResourceType::Buffer)
			{
				renderer.ResourceBarrier(
					static_cast<VulkanBuffer*>(resource), 
					resourceState, 
					resourceShouldBe, 
					resourcePreviousStage, 
					resourceCurrentStage);
			}
		}
	}

	m_ResourcesForTransition.clear();
}

void ResourceStateTrackingManager::AddResourceForTransition(ManagableResource* resource)
{
	m_ResourcesForTransition.push_back(resource);
}