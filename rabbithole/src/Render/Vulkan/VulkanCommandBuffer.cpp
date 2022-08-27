#include "precomp.h"

#include "VulkanCommandBuffer.h"

VulkanCommandBuffer::VulkanCommandBuffer(const VulkanDevice& device, const char* name)
	: m_Device(device)
	, m_Name(name)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = device.GetCommandPool();
	allocInfo.commandBufferCount = 1;

	VULKAN_API_CALL(vkAllocateCommandBuffers(device.GetGraphicDevice(), &allocInfo, &m_CommandBuffer));
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
	vkFreeCommandBuffers(m_Device.GetGraphicDevice(), m_Device.GetCommandPool(), 1, &m_CommandBuffer);
}

void VulkanCommandBuffer::BeginCommandBuffer(bool isSingleTimeCommandBuffer)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (isSingleTimeCommandBuffer)
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VULKAN_API_CALL(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo));
}

void VulkanCommandBuffer::EndCommandBuffer()
{
	VULKAN_API_CALL(vkEndCommandBuffer(m_CommandBuffer));
}

void VulkanCommandBuffer::EndAndSubmitCommandBuffer()
{
	vkEndCommandBuffer(m_CommandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_CommandBuffer;

	vkQueueSubmit(m_Device.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_Device.GetGraphicsQueue());
}
