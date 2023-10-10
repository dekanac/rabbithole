#pragma once

#include <vector>

class ManagableResource;
class Renderer;

class ResourceStateTrackingManager
{
public:
	void CommitBarriers(Renderer& renderer);
	void AddResourceForTransition(ManagableResource* resource);

private:

	std::vector<ManagableResource*> m_ResourcesForTransition;
};