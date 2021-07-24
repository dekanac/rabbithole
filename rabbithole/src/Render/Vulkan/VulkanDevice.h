#pragma once

#include "vk_mem_alloc.h"

struct QueueFamilyIndices 
{
	uint32_t graphicsFamily;
	uint32_t presentFamily;
	bool graphicsFamilyHasValue = false;
	bool presentFamilyHasValue = false;

	bool isComplete() 
	{
		return graphicsFamilyHasValue && presentFamilyHasValue;
	}
};

struct SwapChainSupportDetails 
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class VulkanDevice
{
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = false; //TODO:change this to true
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
	VkCommandPool GetCommandPool() { return m_CommandPool; }
	VkDevice GetGraphicDevice() const { return m_Device; }
	VkSurfaceKHR GetPresentingSurface() { return m_PresetingSurface; }
	VkQueue GetGraphicsQueue() { return m_GraphicsQueue; }
	VkQueue GetPresentQueue() { return m_PresentQueue; }
	VmaAllocator GetVmaAllocator() const { return m_VmaAllocator; }

	SwapChainSupportDetails GetSwapChainSupport() { return QuerySwapChainSupport(m_PhysicalDevice); }
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	QueueFamilyIndices FindPhysicalQueueFamilies() { return FindQueueFamilies(m_PhysicalDevice); }
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	// Buffer Helper Functions
	
	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);
	void CreateImageWithInfo(
		const VkImageCreateInfo& imageInfo,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory& imageMemory);

	VkPhysicalDeviceProperties properties;


private:
	void CreateInstance();
	void SetupDebugMessenger();
	void CreateSurface();
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	void CreateVmaAllocator();
	void CreateCommandPool();

	//helper fuctions
	bool IsDeviceSuitable(VkPhysicalDevice device);
	std::vector<const char*> GetRequiredExtensions();
	bool CheckValidationLayerSupport();
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void HasGflwRequiredInstanceExtensions();
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
	
private:
	VkInstance m_Instance;
	VkDebugUtilsMessengerEXT m_DebugMessenger;
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkCommandPool m_CommandPool;
	VmaAllocator m_VmaAllocator;

	VkDevice m_Device;
	VkSurfaceKHR m_PresetingSurface;
	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;

	const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
	const std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
};

VkBufferUsageFlags GetVkBufferUsageFlags(const BufferUsageFlags usageFlags);
VmaMemoryUsage GetVmaMemoryUsageFrom(const MemoryAccess memoryAccess);
VkDescriptorType GetVkDescriptorTypeFrom(const DescriptorType descriptorSetBinding);