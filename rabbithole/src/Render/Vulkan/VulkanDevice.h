#pragma once

#include "vk_mem_alloc.h"
#include "spirv-reflect/spirv_reflect.h"

class VulkanTexture;
class VulkanBuffer;
class VulkanRenderPass;
class VulkanPipeline;

struct QueueFamilyIndices 
{
	uint32_t graphicsFamily;
	uint32_t presentFamily;
	bool	 graphicsFamilyHasValue = false;
	bool	 presentFamilyHasValue = false;

	bool isComplete() 
	{
		return graphicsFamilyHasValue && presentFamilyHasValue;
	}
};

struct SwapChainSupportDetails 
{
	VkSurfaceCapabilitiesKHR		capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR>	presentModes;
};

class VulkanDevice
{
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true; 
#endif
public:
	VulkanDevice();
	~VulkanDevice();

	//Not copyable or movable -> TODO: add macro for this
	VulkanDevice(const VulkanDevice&) = delete;
	VulkanDevice operator= (const VulkanDevice&) = delete;
	VulkanDevice(VulkanDevice&&) = delete;
	VulkanDevice& operator= (VulkanDevice&&) = delete;

public:
	VkCommandPool	GetCommandPool() const { return m_CommandPool; }
	VkDevice		GetGraphicDevice() const { return m_Device; }
	VkSurfaceKHR	GetPresentingSurface() const { return m_PresetingSurface; }
	VkQueue			GetGraphicsQueue() { return m_GraphicsQueue; }
	VkQueue			GetPresentQueue() const { return m_PresentQueue; }
	VmaAllocator	GetVmaAllocator() const { return m_VmaAllocator; }

	//TODO: see what to do with this
	SwapChainSupportDetails GetSwapChainSupport() { return QuerySwapChainSupport(m_PhysicalDevice); }
	uint32_t				FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	QueueFamilyIndices		FindPhysicalQueueFamilies() { return FindQueueFamilies(m_PhysicalDevice); }
	VkFormat				FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	// Buffer Helper Functions
	VkCommandBuffer		BeginSingleTimeCommands();
	void				EndSingleTimeCommands(VkCommandBuffer commandBuffer);
	void				CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void				CopyBufferToImage(VulkanBuffer* buffer, VulkanTexture* texture);
	void				TransitionImageLayout(VulkanTexture* texture, ResourceState oldLayout, ResourceState newLayout);
	void				CreateImageWithInfo(const VkImageCreateInfo& imageInfo, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

private:
	void CreateInstance();
	void SetupDebugMessenger();
	void CreateSurface();
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	void CreateVmaAllocator();
	void CreateCommandPool();

	//helper fuctions
	bool						IsDeviceSuitable(VkPhysicalDevice device);
	std::vector<const char*>	GetRequiredExtensions();
	bool						CheckValidationLayerSupport();
	QueueFamilyIndices			FindQueueFamilies(VkPhysicalDevice device);
	void						PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void						HasGflwRequiredInstanceExtensions();
	bool						CheckDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainSupportDetails		QuerySwapChainSupport(VkPhysicalDevice device);
	
private:
	VkInstance					m_Instance;
	VkDevice					m_Device;
	VkPhysicalDevice			m_PhysicalDevice = VK_NULL_HANDLE;
	VkSurfaceKHR				m_PresetingSurface;
	VkCommandPool				m_CommandPool;
	VmaAllocator				m_VmaAllocator;
	VkQueue						m_GraphicsQueue;
	VkQueue						m_PresentQueue;
	VkDebugUtilsMessengerEXT	m_DebugMessenger;

	VkPhysicalDeviceProperties	properties;

	const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
	const std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	//TODO: this is for test purposes, this should be moved to future statemanager
	std::unordered_map<std::string, VulkanRenderPass*>	m_RenderPassCollection;
	std::unordered_map<std::string, VulkanPipeline*>	m_PipelineCollection;

public:

	VulkanRenderPass* GetRenderPassFromCollection(std::string name) { return m_RenderPassCollection[name]; }
	void AddRenderPassToCollection(VulkanRenderPass* renderPass, std::string name) { m_RenderPassCollection[name] = renderPass; }
	VulkanPipeline* GetPipelineFromCollection(std::string name) { return m_PipelineCollection[name]; }
	void AddPipelineToCollection(VulkanPipeline* pipeline, std::string name) { m_PipelineCollection[name] = pipeline; }
};

//converter functions //TODO: move to separate file
VkBufferUsageFlags		GetVkBufferUsageFlags(const BufferUsageFlags usageFlags);
VmaMemoryUsage			GetVmaMemoryUsageFrom(const MemoryAccess memoryAccess);
VkDescriptorType		GetVkDescriptorTypeFrom(const DescriptorType descriptorSetBinding);
VkDescriptorType		GetVkDescriptorTypeFrom(const SpvReflectDescriptorType reflectDescriptorType);
VkShaderStageFlagBits	GetVkShaderStageFrom(const ShaderType shaderType);
VkFormat				GetVkFormatFrom(const Format format);
VkColorSpaceKHR			GetVkColorSpaceFrom(const ColorSpace colorSpace);
uint32_t				GetBlockSizeFrom(const VkFormat format);
VkImageType				GetVkImageTypeFrom(const uint32_t imageDepth);
VkImageAspectFlags		GetVkImageAspectFlagsFrom(const VkFormat format);
VkImageLayout			GetVkImageLayoutFrom(const ResourceState resourceState);
VkAccessFlags			GetVkAccessFlagsFrom(const ResourceState resourceState);
VkPipelineStageFlagBits GetGraphicsPipelineStage(const ResourceState resourceState);
VkPipelineStageFlagBits GetTransferPipelineStage(const ResourceState resourceState);
VkPrimitiveTopology		GetVkPrimitiveTopologyFrom(const Topology topology);
VkSampleCountFlagBits	GetVkSampleFlagsFrom(const MultisampleType multiSampleType);
VkCompareOp				GetVkCompareOperationFrom(const CompareOperation compareOperation);
VkStencilOp				GetVkStencilOpFrom(const StencilOperation stencilOperation);
VkFilter				GetVkFilterFrom(const FilterType filterType);
VkSamplerMipmapMode		GetVkMipmapModeFrom(const FilterType filterType);
VkSamplerAddressMode	GetVkAddressModeFrom(const AddressMode addressMode);
VkBlendFactor			GetVkBlendFactorFrom(const BlendValue blendValue);
VkBlendOp				GetVkBlendOpFrom(const BlendOperation blendOperation);
VkVertexInputRate		GetVkVertexInputRateFrom(const VertexInputRate inputRate);
VkImageUsageFlags		GetVkImageUsageFlagsFrom(const ImageUsageFlags usageFlags);
VkBorderColor			GetVkBorderColorFrom(const Color color);

