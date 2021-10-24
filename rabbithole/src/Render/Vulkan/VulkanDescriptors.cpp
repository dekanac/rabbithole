#include "precomp.h"

#include "Shader.h"

VulkanDescriptor::VulkanDescriptor(const VulkanDescriptorInfo& info)
	: m_Info(info)
{
	switch (m_Info.Type)
	{
	case DescriptorType::CombinedSampler:
	{
		CombinedImageSampler* combinedImageSampler = m_Info.combinedImageSampler;
		m_ResourceInfo.m_ResourceInfo.ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		m_ResourceInfo.m_ResourceInfo.ImageInfo.imageView = combinedImageSampler->ImageView->GetImageView();
		m_ResourceInfo.m_ResourceInfo.ImageInfo.sampler = combinedImageSampler->ImageSampler->GetSampler();
		break;
	}
	case DescriptorType::UniformBuffer:
	{
		VulkanBuffer* uniformBuffer = m_Info.buffer;
		m_ResourceInfo.m_ResourceInfo.BufferInfo.buffer = uniformBuffer->GetBuffer();
		m_ResourceInfo.m_ResourceInfo.BufferInfo.offset = 0;
		m_ResourceInfo.m_ResourceInfo.BufferInfo.range = uniformBuffer->GetInfo().size;
		break;
	}
	default:
		ASSERT(false, "Not supported DescriptorType.");
		break;
	}
}

VulkanDescriptorPool::VulkanDescriptorPool(const VulkanDevice* device, const VulkanDescriptorPoolInfo& info)
	: m_Device(device),
	  m_Info(info)
{
	std::vector<VkDescriptorPoolSize> poolSizes{};
	for (const VulkanDescriptorPoolSize descriptorSize : m_Info.DescriptorSizes)
	{
		VkDescriptorPoolSize descriptorPoolSize;
		descriptorPoolSize.descriptorCount = descriptorSize.Count;
		descriptorPoolSize.type = GetVkDescriptorTypeFrom(descriptorSize.Type);
		poolSizes.push_back(descriptorPoolSize);
	}

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
	descriptorPoolCreateInfo.maxSets = m_Info.MaxSets;

	VULKAN_API_CALL(vkCreateDescriptorPool(device->GetGraphicDevice(), &descriptorPoolCreateInfo, nullptr, &m_DescriptorPool));
}

VulkanDescriptorPool::~VulkanDescriptorPool()
{
	vkDestroyDescriptorPool(m_Device->GetGraphicDevice(), m_DescriptorPool, nullptr);
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(const VulkanDevice* device, const std::vector<Shader*> shaders, const char* name)
	: m_Device(device)
{
	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
	for (const Shader* shader : shaders)
	{
		for (const VkDescriptorSetLayoutBinding& binding : shader->GetDescriptorSetLayoutBindings())
		{
			descriptorSetLayoutBindings.push_back(binding);
		}
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());
	descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings.data();

	VULKAN_API_CALL(vkCreateDescriptorSetLayout(m_Device->GetGraphicDevice(), &descriptorSetLayoutCreateInfo, nullptr, &m_Layout));
	
	VkPushConstantRange pushConstantRange{}; //TODO: this is hard coded...need to provide this through arguments
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(SimplePushConstantData);
	
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &m_Layout;

	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

	VULKAN_API_CALL(vkCreatePipelineLayout(m_Device->GetGraphicDevice(), &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
{
	vkDestroyDescriptorSetLayout(m_Device->GetGraphicDevice(), m_Layout, nullptr);
}

VulkanDescriptorSet::VulkanDescriptorSet(const VulkanDevice* device,const VulkanDescriptorPool* desciptorPool,const VulkanDescriptorSetLayout* descriptorSetLayout, const std::vector<VulkanDescriptor*> descriptors, const char* name)
{
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayout->GetLayout();
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.descriptorPool = desciptorPool->GetPool();
	VULKAN_API_CALL(vkAllocateDescriptorSets(device->GetGraphicDevice(), &descriptorSetAllocateInfo, &m_DescriptorSet));
	

	std::vector<VkWriteDescriptorSet> writeDescriptorSets(descriptors.size());
	for (uint32_t i = 0; i < descriptors.size(); ++i)
	{
		VkWriteDescriptorSet& writeDescriptorSet = writeDescriptorSets[i];
		writeDescriptorSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		writeDescriptorSet.dstSet = m_DescriptorSet;
		writeDescriptorSet.dstBinding = i;
		writeDescriptorSet.dstArrayElement = 0;
		writeDescriptorSet.descriptorCount = 1;

		switch (descriptors[i]->GetDescriptorInfo().Type)
		{
		case DescriptorType::CombinedSampler:
 			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
 			writeDescriptorSet.pImageInfo = &((descriptors[i])->GetDescriptorResourceInfo().m_ResourceInfo.ImageInfo);
			break;
		case DescriptorType::UniformBuffer:
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSet.pBufferInfo = &((descriptors[i])->GetDescriptorResourceInfo().m_ResourceInfo.BufferInfo);
			break;
		default:
			ASSERT(false, "Not supported DescriptorType.");
			break;
		}
	}

	uint32_t descriptorWriteCount = static_cast<uint32_t>(writeDescriptorSets.size());
	VkWriteDescriptorSet* descriptorWrites = writeDescriptorSets.data();
	vkUpdateDescriptorSets(device->GetGraphicDevice(), descriptorWriteCount, descriptorWrites, 0, nullptr);
}
