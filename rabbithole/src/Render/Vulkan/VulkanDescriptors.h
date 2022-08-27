#pragma once

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
	const VkDescriptorSet* GetVkHandle() const { return &m_DescriptorSet; }

private:
	VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
};

class DescriptorSetManager
{
public:
	DescriptorSetManager();

	void SetCombinedImageSampler(uint32_t slot, VulkanTexture* texture);
	void SetConstantBuffer(uint32_t slot, VulkanBuffer* buffer, uint64_t offset, uint64_t range);

	void Reset();
	void SetDescriptorSet(VulkanDescriptorSet* set);
	void Commit(VulkanDevice* device);

private:
	static constexpr uint32_t MaxDescriptorEntryCount = 64;

	VulkanDescriptorSet* m_DescriptorSet;
	VkWriteDescriptorSet m_Writes[MaxDescriptorEntryCount];
	uint32_t m_CurrentWriteIndex;
	VkDescriptorImageInfo m_ImageInfos[MaxDescriptorEntryCount];
	uint32_t m_CurrentImageInfoIndex;
	VkDescriptorBufferInfo m_BufferInfos[MaxDescriptorEntryCount];
	uint32_t m_CurrentBufferInfoIndex;

};