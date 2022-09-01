#pragma once

#include "VulkanDevice.h"

class VulkanCommandBuffer
{
public:
	VulkanCommandBuffer(const VulkanDevice& device, const char* name);
	~VulkanCommandBuffer();

	NonCopyableAndMovable(VulkanCommandBuffer);

	VkCommandBuffer GetVkHandle() { return m_CommandBuffer; }
	const char*		GetName() const { return m_Name; }
	
	void BeginCommandBuffer(bool isSingleTimeCommandBuffer = false);
	void EndCommandBuffer();
	void EndAndSubmitCommandBuffer();

private:
	const VulkanDevice&		m_Device;
	VkCommandBuffer			m_CommandBuffer;
	const char*				m_Name;
};