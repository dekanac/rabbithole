#pragma once

#include "Render/Vulkan/VulkanTypes.h"

#include <memory>

class AllocatedResource
{
public:
	AllocatedResource();
	virtual uint32_t GetID() { return m_Id; }

	static uint32_t ms_CurrentId;
protected:
	uint32_t m_Id;
	void UpdateID();
};

class ManagableResource
{
public:

	virtual ResourceState			GetResourceState() const = 0;
	virtual void					SetResourceState(ResourceState state) = 0;

	virtual ResourceState			GetShouldBeResourceState() const = 0;
	virtual void					SetShouldBeResourceState(ResourceState state) = 0;

	virtual ResourceStage			GetCurrentResourceStage() = 0;
	virtual void					SetCurrentResourceStage(ResourceStage stage) = 0;

	virtual ResourceStage			GetPreviousResourceStage() = 0;
	virtual void					SetPreviousResourceStage(ResourceStage stage) = 0;
};