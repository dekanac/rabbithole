#pragma once

#include "vk_mem_alloc.h"
#include "spirv-reflect/spirv_reflect.h"
#include <imgui/backends/imgui_impl_vulkan.h>

#include <unordered_map>
#include "VulkanTypes.h"

class VulkanTexture;
class VulkanBuffer;
class VulkanRenderPass;
class VulkanPipeline;

//#define MUTE_VALIDATION_ERROR_SPAM

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
#ifdef RABBITHOLE_DEBUG
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = false; 
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
	VkCommandPool				GetCommandPool() const { return m_CommandPool; }
	VkDevice					GetGraphicDevice() const { return m_Device; }
	VkSurfaceKHR				GetPresentingSurface() const { return m_PresetingSurface; }
	VkQueue						GetGraphicsQueue() { return m_GraphicsQueue; }
	VkQueue						GetPresentQueue() const { return m_PresentQueue; }
	VmaAllocator				GetVmaAllocator() const { return m_VmaAllocator; }
	VkPhysicalDevice			GetPhysicalDevice() const { return m_PhysicalDevice; }
	VkPhysicalDeviceProperties	GetPhysicalDeviceProperties() const { return properties; }

	//TODO: see what to do with this
	SwapChainSupportDetails GetSwapChainSupport() { return QuerySwapChainSupport(m_PhysicalDevice); }
	uint32_t				FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	QueueFamilyIndices		FindPhysicalQueueFamilies() { return FindQueueFamilies(m_PhysicalDevice); }
	VkFormat				FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	// Buffer Helper Functions
	VkCommandBuffer		BeginSingleTimeCommands();
	void				EndSingleTimeCommands(VkCommandBuffer commandBuffer);
	void				CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void				CopyBufferToImage(VkCommandBuffer commandBuffer, VulkanBuffer* buffer, VulkanTexture* texture, bool copyFirstMipOnly = false);
	void				CopyBufferToImageCubeMap(VkCommandBuffer commandBuffer, VulkanBuffer* buffer, VulkanTexture* texture);
	void				TransitionImageLayout(VkCommandBuffer commandBuffer, VulkanTexture* texture, ResourceState oldLayout, ResourceState newLayout, uint32_t mipLevel = 0, uint32_t mipCount = 1);
	void				CreateImageWithInfo(const VkImageCreateInfo& imageInfo, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void				InitImguiForVulkan(ImGui_ImplVulkan_InitInfo& info);
	
	//debug utils
	void SetObjectName(uint64_t object, VkObjectType objectType, const char* name);
	void BeginLabel(VkCommandBuffer commandBuffer, const char* name);
	void EndLabel(VkCommandBuffer commandBuffer);

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
	void						InitializeFunctionsThroughProcAddr();
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
	const std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_GOOGLE_HLSL_FUNCTIONALITY_1_EXTENSION_NAME, VK_GOOGLE_USER_TYPE_EXTENSION_NAME };

	//TODO: this is for test purposes, this should be moved to future statemanager
	std::unordered_map<std::string, VulkanRenderPass*>	m_RenderPassCollection;
	std::unordered_map<std::string, VulkanPipeline*>	m_PipelineCollection;
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
VkClearValue			GetVkClearColorValueFor(const Format format);
size_t					GetBPPFrom(const Format format);
VkImageUsageFlags		GetVkImageUsageFlagsFrom(const ImageUsageFlags usageFlags);
VkBorderColor			GetVkBorderColorFrom(const Color color);
bool					IsDepthFormat(const Format format);
VkPipelineStageFlags GetVkPipelineStageFromResourceStageAndState(const ResourceStage stage, const ResourceState state);
VkAccessFlags			GetVkAccessFlagsFromResourceState(const ResourceState state);

