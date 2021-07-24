#include "precomp.h"

VulkanDescriptor::VulkanDescriptor(const VulkanDescriptorInfo& info)
{

}

VulkanDescriptor::~VulkanDescriptor()
{

}

VulkanDescriptorPool::VulkanDescriptorPool(const VulkanDevice* device, const VulkanDescriptorPoolInfo& info)
	: m_Device(device),
	  m_Info(info)
{
	VkDescriptorPoolSize poolSize{};
	poolSize.type = GetVkDescriptorTypeFrom(info.type);
	poolSize.descriptorCount = static_cast<uint32_t>(info.descriptorCount);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = info.poolSizeCount;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = info.maxSets;

	if (vkCreateDescriptorPool(device->GetGraphicDevice(), &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) 
	{
		LOG_ERROR("failed to create descriptor pool!");
	}
}

VulkanDescriptorPool::~VulkanDescriptorPool()
{
	vkDestroyDescriptorPool(m_Device->GetGraphicDevice(), m_DescriptorPool, nullptr);
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(const VulkanDevice* device, const VulkanDescriptorSetLayoutInfo& info)
	:	m_Device(device),
		m_Info(info)
{
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = info.bindings.size();
	layoutInfo.pBindings = info.bindings.data();

	if (vkCreateDescriptorSetLayout(m_Device->GetGraphicDevice(), &layoutInfo, nullptr, &m_Layout) != VK_SUCCESS) 
	{
		LOG_ERROR("failed to create descriptor set layout!");
	}
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
{
	vkDestroyDescriptorSetLayout(m_Device->GetGraphicDevice(), m_Layout, nullptr);
}
