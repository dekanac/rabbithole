#include "common.h"
#include "vulkan/precomp.h"
#include "ResourceStateTracking.h"
#include "Renderer.h"
#include "Vulkan/Resource.h"

void ResourceStateTrackingManager::TransitionResources()
{
	for (auto resource : m_ResourcesForTransition)
	{
		ResourceState resourceState = resource->GetResourceState();
		ResourceState resourceShouldBe = resource->GetShouldBeResourceState();

		if (!(resourceState == resourceShouldBe))
		{
			//for now only textures support states
			Renderer::instance().ResourceBarrier((VulkanTexture*)resource, resourceState, resourceShouldBe);
		}
	}
}

void ResourceStateTrackingManager::AddResourceForTransition(ManagableResource* resource)
{
	m_ResourcesForTransition.push_back(resource);
}

void ResourceStateTrackingManager::Reset()
{
	m_ResourcesForTransition.clear();
}
