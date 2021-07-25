#pragma once

class VulkanDevice;
class VulkanBuffer;
class Shader;

struct VulkanDescriptorInfo
{
	//TODO: add support for images and samplers
	DescriptorType	Type;
	uint32_t		Binding;

	VulkanBuffer*	buffer;
};

struct DescriptorResourceInfo
{
	union
	{
		VkDescriptorImageInfo	ImageInfo;
		VkDescriptorBufferInfo	BufferInfo;
	} m_ResourceInfo;
};

class VulkanDescriptor
{
public:
	VulkanDescriptor(const VulkanDescriptorInfo& info);

	inline VulkanDescriptorInfo			GetDescriptorInfo() { return m_Info; }
	inline const DescriptorResourceInfo GetDescriptorResourceInfo() const { return m_ResourceInfo; }

private:
	VulkanDescriptorInfo	m_Info{};
	DescriptorResourceInfo	m_ResourceInfo{};

};

struct 	VulkanDescriptorPoolInfo
{
	DescriptorType	type;
	uint32_t		descriptorCount;
	uint32_t		poolSizeCount;
	uint32_t		maxSets;
};

class VulkanDescriptorPool
{
public:
	VulkanDescriptorPool(const VulkanDevice* device, const VulkanDescriptorPoolInfo& info);
	~VulkanDescriptorPool();

	const VkDescriptorPool GetPool() const { return m_DescriptorPool; }
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
	inline const VkPipelineLayout*		GetPipelineLayout() const { return &m_PipelineLayout; }
	
private:
	const VulkanDevice*		m_Device;
	VkDescriptorSetLayout	m_Layout;
	VkPipelineLayout		m_PipelineLayout;

};

class VulkanDescriptorSet
{
public:
	VulkanDescriptorSet(
		const VulkanDevice* device,
		const VulkanDescriptorPool* desciptorPool,
		const VulkanDescriptorSetLayout* descriptorSetLayout,
		const std::vector<VulkanDescriptor*> descriptors,
		const char* name);

public:
	inline const VkDescriptorSet* GetDescriptorSet() const { return &m_DescriptorSet; }

private:
	VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
};