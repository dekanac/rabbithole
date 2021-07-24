#pragma once

class VulkanDevice;
class VulkanBuffer;

struct VulkanDescriptorInfo
{
	//TODO: add support for images and samplers
	VulkanBuffer* buffer;
};

class VulkanDescriptor
{
public:
	VulkanDescriptor(const VulkanDescriptorInfo& info);
	~VulkanDescriptor();

	inline VulkanDescriptorInfo GetDescriptorInfo() { return m_Info; }

private:
	VulkanDescriptorInfo m_Info{};


};

struct 	VulkanDescriptorPoolInfo
{
	DescriptorType type;
	uint32_t descriptorCount;

	uint32_t poolSizeCount;
	uint32_t maxSets;

};

class VulkanDescriptorPool
{
public:
	VulkanDescriptorPool(const VulkanDevice* device, const VulkanDescriptorPoolInfo& info);
	~VulkanDescriptorPool();

	inline const VkDescriptorPool GetPool() { return m_DescriptorPool; }
private:
	const VulkanDevice* m_Device;
	VulkanDescriptorPoolInfo m_Info{};

	VkDescriptorPool m_DescriptorPool;
};

struct VulkanDescriptorSetLayoutInfo
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
};

class VulkanDescriptorSetLayout
{
public:
	VulkanDescriptorSetLayout(const VulkanDevice* device, const VulkanDescriptorSetLayoutInfo& info);
	~VulkanDescriptorSetLayout();

	inline const VulkanDescriptorSetLayoutInfo GetInfo() { return m_Info; }
	inline const VkDescriptorSetLayout GetLayout() { return m_Layout; }
	
private:
	const VulkanDevice* m_Device;
	const VulkanDescriptorSetLayoutInfo m_Info;

	VkDescriptorSetLayout m_Layout;

};