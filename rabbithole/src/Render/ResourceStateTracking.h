#pragma once

#include <vector>

class ManagableResource;

class ResourceStateTrackingManager
{
	SingletonClass(ResourceStateTrackingManager)

public:
	void CommitBarriers();
	void AddResourceForTransition(ManagableResource* resource);
	void Reset();

private:

	std::vector<ManagableResource*> m_ResourcesForTransition;
};

#define RSTManager ResourceStateTrackingManager::instance()