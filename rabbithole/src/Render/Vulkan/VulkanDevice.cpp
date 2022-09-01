#include "precomp.h"

#include "vk_mem_alloc.h"

#include "Render/Window.h"
#include "Render/Converters.h"
// std headers
#include <cstring>
#include <iostream>
#include <set>
#include <unordered_set>

std::vector<rabbitVec4f> g_Colors = { YELLOW_COLOR, RED_COLOR, BLUE_COLOR, PUPRPLE_COLOR, GREEN_COLOR };


rabbitVec4f GetNextColor()
{
	static int i = 0;

	i = (i < (g_Colors.size() - 1)) ? ++i : 0;

	return g_Colors[i];
}

// local callback functions
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) 
{
	if (strstr(pCallbackData->pMessage, "Error") != NULL)
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	}

	return VK_FALSE;
}

PFN_vkCmdBeginDebugUtilsLabelEXT pfnCmdBeginDebugUtilsLabelEXT;
PFN_vkCmdEndDebugUtilsLabelEXT pfnCmdEndDebugUtilsLabelEXT;
PFN_vkDebugMarkerSetObjectTagEXT pfnDebugMarkerSetObjectTag;
PFN_vkSetDebugUtilsObjectNameEXT pfnDebugUtilsObjectName;

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) 
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	
	if (func != nullptr) 
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else 
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) 
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	
	if (func != nullptr) 
		func(instance, debugMessenger, pAllocator);
}

// class member functions
VulkanDevice::VulkanDevice() 
{
	CreateInstance();
	SetupDebugMessenger();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateVmaAllocator();
	CreateCommandPool();
	InitializeFunctionsThroughProcAddr();
}

VulkanDevice::~VulkanDevice() 
{
	vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
	vkDestroyDevice(m_Device, nullptr);

	if (enableValidationLayers) 
	{
		DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(m_Instance, m_PresetingSurface, nullptr);
	vkDestroyInstance(m_Instance, nullptr);
}

void VulkanDevice::CreateInstance() 
{
	if (enableValidationLayers && !CheckValidationLayerSupport()) 
	{
		LOG_ERROR("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Rabbithole";
	appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);
	appInfo.pEngineName = "Rabbithole Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 3, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = GetRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (enableValidationLayers) 
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
		createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else 
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS) 
	{
		LOG_ERROR("failed to create instance!");
	}

	HasGflwRequiredInstanceExtensions();
}

void VulkanDevice::PickPhysicalDevice() 
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
	if (deviceCount == 0) 
	{
		LOG_ERROR("failed to find GPUs with Vulkan support!");
	}
	std::cout << "Device count: " << deviceCount << std::endl;
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

	for (const auto& device : devices) 
	{
		if (IsDeviceSuitable(device)) 
		{
			m_PhysicalDevice = device;
			break;
		}
	}

	if (m_PhysicalDevice == VK_NULL_HANDLE) 
	{
		LOG_ERROR("failed to find a suitable GPU!");
	}

	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_Properties);
	std::cout << "physical device: " << m_Properties.deviceName << std::endl;
}

void VulkanDevice::CreateLogicalDevice() 
{
	QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) 
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
	createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

	// might not really be necessary anymore because device specific validation layers
	// have been deprecated
	if (enableValidationLayers) 
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
		createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
	}
	else 
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS) 
	{
		LOG_ERROR("failed to create logical device!");
	}

	vkGetDeviceQueue(m_Device, indices.graphicsFamily, 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_Device, indices.presentFamily, 0, &m_PresentQueue);
}

void VulkanDevice::CreateVmaAllocator()
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0;
	allocatorInfo.instance = m_Instance;
	allocatorInfo.physicalDevice = m_PhysicalDevice;
	allocatorInfo.device = m_Device;

	vmaCreateAllocator(&allocatorInfo, &m_VmaAllocator);
}

void VulkanDevice::CreateCommandPool() 
{
	QueueFamilyIndices queueFamilyIndices = FindPhysicalQueueFamilies();

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags =
		VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) 
	{
		LOG_ERROR("failed to create command pool!");
	}
}

void VulkanDevice::CreateSurface() 
{
	if (glfwCreateWindowSurface(m_Instance, Window::instance().GetNativeWindowHandle(), nullptr, &m_PresetingSurface) != VK_SUCCESS) 
	{
		LOG_ERROR("failed to create window surface!");
	}
}

bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice device) 
{
	QueueFamilyIndices indices = FindQueueFamilies(device);

	bool extensionsSupported = CheckDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) 
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.isComplete() && extensionsSupported && swapChainAdequate &&
		supportedFeatures.samplerAnisotropy;
}

void VulkanDevice::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) 
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
	createInfo.pUserData = nullptr;  // Optional
}

void VulkanDevice::SetupDebugMessenger() 
{
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(createInfo);
	if (CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS) 
	{
		LOG_ERROR("failed to set up debug messenger!");
	}
}

bool VulkanDevice::CheckValidationLayerSupport() 
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : m_ValidationLayers) 
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) 
		{
			if (strcmp(layerName, layerProperties.layerName) == 0) 
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound) 
		{
			return false;
		}
	}

	return true;
}

std::vector<const char*> VulkanDevice::GetRequiredExtensions() 
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	//fsr with renderdoc crashes without this extension
	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	return extensions;
}

void VulkanDevice::HasGflwRequiredInstanceExtensions() 
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	std::cout << "available extensions:" << std::endl;
	std::unordered_set<std::string> available;
	for (const auto& extension : extensions) 
	{
		std::cout << "\t" << extension.extensionName << std::endl;
		available.insert(extension.extensionName);
	}

	std::cout << "required extensions:" << std::endl;
	auto requiredExtensions = GetRequiredExtensions();
	for (const auto& required : requiredExtensions) 
	{
		std::cout << "\t" << required << std::endl;
		if (available.find(required) == available.end()) 
		{
			LOG_ERROR("Missing required glfw extension");
		}
	}
}

bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device) 
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

	for (const auto& extension : availableExtensions) 
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice device) 
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) 
	{
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
		{
			indices.graphicsFamily = i;
			indices.graphicsFamilyHasValue = true;
		}
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_PresetingSurface, &presentSupport);
		if (queueFamily.queueCount > 0 && presentSupport) 
		{
			indices.presentFamily = i;
			indices.presentFamilyHasValue = true;
		}
		if (indices.isComplete()) 
		{
			break;
		}

		i++;
	}

	return indices;
}

SwapChainSupportDetails VulkanDevice::QuerySwapChainSupport(VkPhysicalDevice device) 
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_PresetingSurface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_PresetingSurface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_PresetingSurface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_PresetingSurface, &presentModeCount, nullptr);

	if (presentModeCount != 0) 
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			device,
			m_PresetingSurface,
			&presentModeCount,
			details.presentModes.data());
	}
	return details;
}

void VulkanDevice::InitializeFunctionsThroughProcAddr()
{
	pfnDebugMarkerSetObjectTag = (PFN_vkDebugMarkerSetObjectTagEXT)vkGetInstanceProcAddr(m_Instance, "vkDebugMarkerSetObjectTagEXT");
	pfnDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(m_Device, "vkSetDebugUtilsObjectNameEXT");
	pfnCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(m_Instance, "vkCmdBeginDebugUtilsLabelEXT");
	pfnCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(m_Instance, "vkCmdEndDebugUtilsLabelEXT");
}

VkFormat VulkanDevice::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) 
{
	for (VkFormat format : candidates) 
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) 
		{
			return format;
		}
	}
	LOG_ERROR("failed to find supported format!");
	return VK_FORMAT_UNDEFINED;
}

uint32_t VulkanDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) 
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
	{
		if ((typeFilter & (1 << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & properties) == properties) 
		{
			return i;
		}
	}
	LOG_ERROR("failed to find suitable memory type!");
	return UINT_MAX;
}

void VulkanDevice::CopyBuffer(VulkanBuffer& srcBuffer, VulkanBuffer& dstBuffer, uint64_t size, uint64_t srcOffset, uint64_t dstOffset)
{
	VulkanCommandBuffer tempCommandBuffer(*this, "Temp Copy Buffer Command Buffer");
	tempCommandBuffer.BeginCommandBuffer(true);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = srcOffset;
	copyRegion.dstOffset = dstOffset;
	copyRegion.size = size;
	vkCmdCopyBuffer(GET_VK_HANDLE(tempCommandBuffer), GET_VK_HANDLE(srcBuffer), GET_VK_HANDLE(dstBuffer), 1, &copyRegion);

	tempCommandBuffer.EndAndSubmitCommandBuffer();
}

void VulkanDevice::CopyBufferToImage(VulkanCommandBuffer& commandBuffer, VulkanBuffer* buffer, VulkanTexture* texture, bool copyFirstMipOnly)
{
	ImageRegion texRegion = texture->GetRegion();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = texRegion.Subresource.MipSlice;
	region.imageSubresource.baseArrayLayer = texRegion.Subresource.ArraySlice;
	region.imageSubresource.layerCount = copyFirstMipOnly ? 1 : texRegion.Subresource.MipSize;

	region.imageOffset = { texRegion.Offset.X, texRegion.Offset.Y, texRegion.Offset.Z };
	region.imageExtent = { texRegion.Extent.Width, texRegion.Extent.Height, 1 };

	vkCmdCopyBufferToImage(
		GET_VK_HANDLE(commandBuffer),
		GET_VK_HANDLE_PTR(buffer),
		GET_VK_HANDLE_PTR(texture->GetResource()),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region);

	texture->SetCurrentResourceStage(ResourceStage::Transfer);
}

void VulkanDevice::CopyBufferToImageCubeMap(VulkanCommandBuffer& commandBuffer, VulkanBuffer* buffer, VulkanTexture* texture)
{
	ImageRegion texRegion = texture->GetRegion();
	auto width = texRegion.Extent.Width;
	auto height = texRegion.Extent.Height;

	std::vector<VkBufferImageCopy> bufferCopyRegions;
	uint32_t offset = 0;

	for (uint32_t face = 0; face < 6; face++)
	{
		{
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = face;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = width;
			bufferCopyRegion.imageExtent.height = height;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = face * width * height * 4;
			bufferCopyRegions.push_back(bufferCopyRegion);
		}
	}

	vkCmdCopyBufferToImage(
		GET_VK_HANDLE(commandBuffer),
		GET_VK_HANDLE_PTR(buffer),
		GET_VK_HANDLE_PTR(texture->GetResource()),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		6,
		bufferCopyRegions.data());

	texture->SetCurrentResourceStage(ResourceStage::Transfer);
}

void VulkanDevice::CopyImage(VulkanCommandBuffer& commandBuffer, VulkanTexture* src, VulkanTexture* dst)
{
	VkImageCopy imageCopyRegion{};
	imageCopyRegion.srcSubresource.aspectMask = GetVkImageAspectFlagsFrom(GetVkFormatFrom(src->GetFormat()));
	imageCopyRegion.srcSubresource.layerCount = src->GetRegion().Subresource.ArraySize;
	imageCopyRegion.dstSubresource.aspectMask = GetVkImageAspectFlagsFrom(GetVkFormatFrom(dst->GetFormat()));
	imageCopyRegion.dstSubresource.layerCount = dst->GetRegion().Subresource.ArraySize;
	imageCopyRegion.extent.width = src->GetWidth();
	imageCopyRegion.extent.height = src->GetHeight();
	imageCopyRegion.extent.depth = src->GetDepth();

	ResourceStage srcStage = src->GetCurrentResourceStage();
	ResourceStage dstStage = dst->GetCurrentResourceStage();

	ResourceState srcState = src->GetResourceState();
	ResourceState dstState = dst->GetResourceState();

	if (srcState != ResourceState::TransferSrc)
		ResourceBarrier(commandBuffer, src, srcState, ResourceState::TransferSrc, srcStage, ResourceStage::Transfer);
	if (dstState != ResourceState::TransferDst)
		ResourceBarrier(commandBuffer, dst, dstState, ResourceState::TransferDst, dstStage, ResourceStage::Transfer);

	vkCmdCopyImage(
		GET_VK_HANDLE(commandBuffer),
		GET_VK_HANDLE_PTR(src->GetResource()),
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		GET_VK_HANDLE_PTR(dst->GetResource()),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&imageCopyRegion);

	if (srcState != ResourceState::TransferSrc)
		ResourceBarrier(commandBuffer, src, ResourceState::TransferSrc, srcState, ResourceStage::Transfer, srcStage);
	if (dstState != ResourceState::TransferDst)
		ResourceBarrier(commandBuffer, dst, ResourceState::TransferDst, dstState, ResourceStage::Transfer, dstStage);
}


void VulkanDevice::InitImguiForVulkan(ImGui_ImplVulkan_InitInfo& info)
{
	info.Instance = m_Instance;
	info.PhysicalDevice = m_PhysicalDevice;
	info.Device = m_Device;
	info.Queue = m_GraphicsQueue;
}

void VulkanDevice::SetObjectName(uint64_t object, VkObjectType objectType, const char* name)
{
#ifdef RABBITHOLE_DEBUG
	// Check for a valid function pointer
	if (pfnDebugUtilsObjectName)
	{
		VkDebugUtilsObjectNameInfoEXT nameInfo = {};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = objectType;
		nameInfo.objectHandle = object;
		nameInfo.pObjectName = name;
		pfnDebugUtilsObjectName(m_Device, &nameInfo);
	}
#endif // RABBITHOLE_DEBUG
}

void VulkanDevice::BeginLabel(VulkanCommandBuffer& commandBuffer, const char* name)
{
#ifdef RABBITHOLE_DEBUG
	glm::vec4 color = GetNextColor();
	VkDebugUtilsLabelEXT labelInfo{};
	labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	labelInfo.color[0] = color[0];
	labelInfo.color[1] = color[1];
	labelInfo.color[2] = color[2];
	labelInfo.color[3] = color[3];
	labelInfo.pLabelName = name;

	pfnCmdBeginDebugUtilsLabelEXT(GET_VK_HANDLE(commandBuffer), &labelInfo);
#endif // RABBITHOLE_DEBUG
}

void VulkanDevice::EndLabel(VulkanCommandBuffer& commandBuffer)
{
#ifdef RABBITHOLE_DEBUG
	pfnCmdEndDebugUtilsLabelEXT(GET_VK_HANDLE(commandBuffer));
#endif // RABBITHOLE_DEBUG
}

void VulkanDevice::ResourceBarrier(VulkanCommandBuffer& commandBuffer, VulkanTexture* texture, ResourceState oldLayout, ResourceState newLayout, ResourceStage srcStage, ResourceStage dstStage, uint32_t mipLevel, uint32_t mipCount)
{
	uint32_t arraySize = texture->GetResource()->GetInfo().ArraySize;

	bool isDepth = GetVkImageAspectFlagsFrom(GetVkFormatFrom(texture->GetFormat())) == VK_IMAGE_ASPECT_DEPTH_BIT;

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = GetVkImageLayoutFrom(oldLayout);
	barrier.newLayout = GetVkImageLayoutFrom(newLayout);
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = GET_VK_HANDLE_PTR(texture->GetResource());
	barrier.subresourceRange.aspectMask = GetVkImageAspectFlagsFrom(GetVkFormatFrom(texture->GetFormat()));
	barrier.subresourceRange.baseMipLevel = mipLevel;
	barrier.subresourceRange.levelCount = mipCount;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = arraySize;

	barrier.srcAccessMask = GetVkAccessFlagsFromResourceState(oldLayout);
	barrier.dstAccessMask = GetVkAccessFlagsFromResourceState(newLayout);

	VkPipelineStageFlags sourceStage = GetVkPipelineStageFromResourceStageAndState(srcStage, oldLayout);
	VkPipelineStageFlags destinationStage = GetVkPipelineStageFromResourceStageAndState(dstStage, newLayout);

	vkCmdPipelineBarrier(
		GET_VK_HANDLE(commandBuffer),
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	texture->SetResourceState(newLayout);
	texture->SetCurrentResourceStage(dstStage);
}

void VulkanDevice::CopyImageToBuffer(VulkanCommandBuffer& commandBuffer, VulkanTexture* texture, VulkanBuffer* buffer)
{
	ImageRegion texRegion = texture->GetRegion();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = texRegion.Subresource.MipSlice;
	region.imageSubresource.baseArrayLayer = texRegion.Subresource.ArraySlice;
	region.imageSubresource.layerCount = texRegion.Subresource.MipSize;

	region.imageOffset = { texRegion.Offset.X, texRegion.Offset.Y, texRegion.Offset.Z };
	region.imageExtent = { texRegion.Extent.Width, texRegion.Extent.Height, 1 };

	ResourceStage srcStage = texture->GetCurrentResourceStage();
	ResourceState srcState = texture->GetResourceState();

	if (srcState != ResourceState::TransferSrc)
		ResourceBarrier(commandBuffer, texture, srcState, ResourceState::TransferSrc, srcStage, ResourceStage::Transfer);
	
	vkCmdCopyImageToBuffer(GET_VK_HANDLE(commandBuffer), GET_VK_HANDLE_PTR(texture->GetResource()), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, GET_VK_HANDLE_PTR(buffer), 1, &region);

	if (srcState != ResourceState::TransferSrc)
		ResourceBarrier(commandBuffer, texture, ResourceState::TransferSrc,srcState , ResourceStage::Transfer, srcStage);
}
