#include "common.h"

#include "ResourceStateTracking.h"
#include "Renderer.h"
#include "Resource.h"

void ResourceStateTrackingManager::CommitBarriers()
{
	for (auto resource : m_ResourcesForTransition)
	{
		ResourceState resourceState = resource->GetResourceState();
		ResourceState resourceShouldBe = resource->GetShouldBeResourceState();
		ResourceStage resourcePreviousStage = resource->GetPreviousResourceStage();
		ResourceStage resourceCurrentStage = resource->GetCurrentResourceStage();

		if (!(resourceState == resourceShouldBe))
		{
			//for now only textures support states
			Renderer::instance().ResourceBarrier((VulkanTexture*)resource, resourceState, resourceShouldBe, resourcePreviousStage, resourceCurrentStage);
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
