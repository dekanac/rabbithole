#pragma once

#include <vector>

class ManagableResource;

class ResourceStateTrackingManager
{
	SingletonClass(ResourceStateTrackingManager)

public:
	void TransitionResources();
	void AddResourceForTransition(ManagableResource* resource);
	void Reset();

private:

	std::vector<ManagableResource*> m_ResourcesForTransition;
};

#define RSTManager ResourceStateTrackingManager::instance()