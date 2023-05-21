#pragma once
#include "common.h"

#include "vk_mem_alloc.h"
#include "spirv-reflect/spirv_reflect.h"
#include <imgui/backends/imgui_impl_vulkan.h>

#include <unordered_map>
#include "VulkanTypes.h"

class VulkanTexture;
class VulkanBuffer;
class VulkanRenderPass;
class VulkanPipeline;
class VulkanCommandBuffer;

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

	NonCopyableAndMovable(VulkanDevice);
public:
	VkCommandPool				GetCommandPool() const { return m_CommandPool; }
	VkDevice					GetGraphicDevice() const { return m_Device; }
	VkSurfaceKHR				GetPresentingSurface() const { return m_PresetingSurface; }
	VkQueue						GetGraphicsQueue() const { return m_GraphicsQueue; }
	VkQueue						GetPresentQueue() const { return m_PresentQueue; }
	VkInstance					GetInstance() const { return m_Instance; }
	VmaAllocator				GetVmaAllocator() const { return m_VmaAllocator; }
	VkPhysicalDevice			GetPhysicalDevice() const { return m_PhysicalDevice; }
	VkPhysicalDeviceProperties2	GetPhysicalDeviceProperties() const { return m_Properties; }
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR GetRayTracingProperties() const { return m_RaytracingProperties; }
	VkPhysicalDeviceAccelerationStructurePropertiesKHR GetAccelerationStructureProperties() const { return m_AccelerationStructureProperties; }
	SwapChainSupportDetails		GetSwapChainSupport() { return QuerySwapChainSupport(m_PhysicalDevice); }
	QueueFamilyIndices			FindPhysicalQueueFamilies() { return FindQueueFamilies(m_PhysicalDevice); }
	VkFormat					FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);


	// Buffer Helper Functions
	void CopyBuffer(VulkanCommandBuffer& commandBuffer, VulkanBuffer& srcBuffer, VulkanBuffer& dstBuffer, uint64_t size, uint64_t srcOffset = 0, uint64_t dstOffset = 0);
	void CopyBufferToImage(VulkanCommandBuffer& commandBuffer, VulkanBuffer* buffer, VulkanTexture* texture, uint32_t mipCount = 1);
	void CopyBufferToImageCubeMap(VulkanCommandBuffer& commandBuffer, VulkanBuffer* buffer, VulkanTexture* texture);
	void CopyImageToBuffer(VulkanCommandBuffer& commandBuffer, VulkanTexture* texture, VulkanBuffer* buffer);
	void CopyImage(VulkanCommandBuffer& commandBuffer, VulkanTexture* src, VulkanTexture* dst);
	void ResourceBarrier(VulkanCommandBuffer& commandBuffer, VulkanTexture* texture, ResourceState oldLayout, ResourceState newLayout, ResourceStage srcStage, ResourceStage dstStage, uint32_t mipLevel = 0, uint32_t mipCount = UINT32_MAX);
	void ResourceBarrier(VulkanCommandBuffer& commandBuffer, VulkanBuffer* buffer, ResourceState oldLayout, ResourceState newLayout, ResourceStage srcStage, ResourceStage dstStage);
		 
	void InitImguiForVulkan(ImGui_ImplVulkan_InitInfo& info);

	//debug utils
	void SetObjectName(uint64_t object, VkObjectType objectType, const char* name);
	void BeginLabel(VulkanCommandBuffer& commandBuffer, const char* name);
	void EndLabel(VulkanCommandBuffer& commandBuffer);

	PFN_vkGetAccelerationStructureBuildSizesKHR pfnGetAccelerationStructureBuildSizesKHR;
	PFN_vkCmdBuildAccelerationStructuresKHR pfnCmdBuildAccelerationStructuresKHR;
	PFN_vkCreateRayTracingPipelinesKHR pfnCreateRayTracingPipelinesKHR;
	PFN_vkGetRayTracingShaderGroupHandlesKHR pfnGetRayTracingShaderGroupHandlesKHR;
	PFN_vkCmdTraceRaysKHR pfnCmdTraceRaysKHR;
	PFN_vkDestroyAccelerationStructureKHR pfnDestroyAccelerationStructureKHR;

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
	uint32_t					FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	
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
	VkPhysicalDeviceProperties2	m_Properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_RaytracingProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
	VkPhysicalDeviceAccelerationStructurePropertiesKHR m_AccelerationStructureProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};

	const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
	const std::vector<const char*> m_DeviceExtensions = { 
		VK_KHR_SWAPCHAIN_EXTENSION_NAME 
		, VK_GOOGLE_HLSL_FUNCTIONALITY_1_EXTENSION_NAME
		, VK_GOOGLE_USER_TYPE_EXTENSION_NAME 
#ifdef VULKAN_HWRT
		, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME
		, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
		, VK_KHR_RAY_QUERY_EXTENSION_NAME
		, VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME
		, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
		, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
#endif
	};
};

