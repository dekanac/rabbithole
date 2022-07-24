#pragma once

struct VulkanCommandBufferInfo
{

};

class VulkanCommandBuffer
{
public:
	VulkanCommandBuffer(VulkanDevice& device, VulkanCommandBufferInfo& info, const char* name);
};