#pragma once

#include "Render/Raytracing.h"

class VulkanDevice;
class VulkanBuffer;
class VulkanImageView;
class VulkanImageSampler;
class Shader;

struct VulkanDescriptorInfo
{
	DescriptorType	Type;
	uint32_t		Binding;

	VulkanBuffer*			buffer; 
	VulkanImageView*		imageView;
	VulkanImageSampler*		imageSampler;
#if defined(VULKAN_HWRT)
	RayTracing::AccelerationStructure* accelerationStructure;
#endif
};

struct DescriptorResourceInfo
{
	union
	{
		VkDescriptorImageInfo		ImageInfo;
		VkDescriptorBufferInfo		BufferInfo;
		VkAccelerationStructureKHR	AccelerationStructureHandle;
	} m_ResourceInfo;
};

class VulkanDescriptor
{
public:
	VulkanDescriptor(const VulkanDescriptorInfo& info);

	inline VulkanDescriptorInfo		GetDescriptorInfo() const { return m_Info; }
	inline DescriptorResourceInfo	GetDescriptorResourceInfo() const { return m_ResourceInfo; }

private:
	VulkanDescriptorInfo	m_Info{};
	DescriptorResourceInfo	m_ResourceInfo{};

};

struct VulkanDescriptorPoolSize
{
	DescriptorType Type;
	uint32_t Count;
};

struct 	VulkanDescriptorPoolInfo
{
	std::vector<VulkanDescriptorPoolSize>	  DescriptorSizes;
	uint32_t								  MaxSets;
};

class VulkanDescriptorPool
{
public:
	VulkanDescriptorPool(const VulkanDevice* device, const VulkanDescriptorPoolInfo& info);
	~VulkanDescriptorPool();

	const VkDescriptorPool GetVkHandle() const { return m_DescriptorPool; }
private:
	const VulkanDevice*		 m_Device;
	VulkanDescriptorPoolInfo m_Info{};
	VkDescriptorPool		 m_DescriptorPool;
};

class VulkanDescriptorSetLayout
{
public:
	VulkanDescriptorSetLayout(const VulkanDevice* device,
		const std::vector<Shader*> shaders,
		const char* name);
	~VulkanDescriptorSetLayout();

	inline const VkDescriptorSetLayout* GetLayout() const { return &m_Layout; }
	
private:
	const VulkanDevice*		m_Device;

	VkDescriptorSetLayout	m_Layout;
};

class VulkanDescriptorSet
{
public:
	VulkanDescriptorSet(
		const VulkanDevice* device,
		const VulkanDescriptorPool* desciptorPool,
		const VulkanDescriptorSetLayout* descriptorSetLayout,
		const std::vector<VulkanDescriptor*>& descriptors,
		const char* name);

public:
	const VkDescriptorSet* GetVkHandle() const { return &m_DescriptorSet; }

private:
	VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
};