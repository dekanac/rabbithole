#include "precomp.h"

#include "vk_mem_alloc.h"

#include "..\Window.h"
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
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

PFN_vkCmdBeginDebugUtilsLabelEXT pfnCmdBeginDebugUtilsLabelEXT;
PFN_vkCmdEndDebugUtilsLabelEXT pfnCmdEndDebugUtilsLabelEXT;
PFN_vkDebugMarkerSetObjectTagEXT pfnDebugMarkerSetObjectTag;
PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName;

VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance,
		"vkCreateDebugUtilsMessengerEXT");
	
	if (func != nullptr) 
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator) 
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance,
		"vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
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
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Rabbithole Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

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

	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
	std::cout << "physical device: " << properties.deviceName << std::endl;
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
#ifndef MUTE_VALIDATION_ERROR_SPAM
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
#endif
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
	vkEnumerateDeviceExtensionProperties(
		device,
		nullptr,
		&extensionCount,
		availableExtensions.data());

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
	pfnDebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(m_Device, "vkDebugMarkerSetObjectNameEXT");
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
		else if (
			tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) 
		{
			return format;
		}
	}
	LOG_ERROR("failed to find supported format!");
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
}

VkCommandBuffer VulkanDevice::BeginSingleTimeCommands() 
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_CommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	return commandBuffer;
}

void VulkanDevice::EndSingleTimeCommands(VkCommandBuffer commandBuffer) 
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_GraphicsQueue);

	vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &commandBuffer);
}

void VulkanDevice::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;  // Optional
	copyRegion.dstOffset = 0;  // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(commandBuffer);
}

void VulkanDevice::CopyBufferToImage(VulkanBuffer* buffer, VulkanTexture* texture)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

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

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer->GetBuffer(),
		texture->GetResource()->GetImage(),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region);

	EndSingleTimeCommands(commandBuffer);
}

void VulkanDevice::CopyBufferToImageCubeMap(VulkanBuffer* buffer, VulkanTexture* texture)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

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
		commandBuffer,
		buffer->GetBuffer(),
		texture->GetResource()->GetImage(),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		6,
		bufferCopyRegions.data());

	EndSingleTimeCommands(commandBuffer);
}


void VulkanDevice::TransitionImageLayout(VulkanTexture* texture, ResourceState oldLayout, ResourceState newLayout)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = GetVkImageLayoutFrom(oldLayout);
	barrier.newLayout = GetVkImageLayoutFrom(newLayout);
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = texture->GetResource()->GetImage();
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = (IsFlagSet(texture->GeFlags() & TextureFlags::CubeMap)) ? 6 : 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == ResourceState::None && newLayout == ResourceState::TransferDst) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
 	else if (oldLayout == ResourceState::None && newLayout == ResourceState::DepthStencilWrite)
 	{
 		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
 
 		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
 		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT  | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
 	}
	else if (oldLayout == ResourceState::TransferDst && newLayout == ResourceState::GenericRead) 
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == ResourceState::None && newLayout == ResourceState::RenderTarget)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == ResourceState::RenderTarget && newLayout == ResourceState::GenericRead)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	else 
	{
		LOG_ERROR("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	EndSingleTimeCommands(commandBuffer);
}

void VulkanDevice::CreateImageWithInfo(
	const VkImageCreateInfo& imageInfo,
	VkMemoryPropertyFlags properties,
	VkImage& image,
	VkDeviceMemory& imageMemory) 
{
	if (vkCreateImage(m_Device, &imageInfo, nullptr, &image) != VK_SUCCESS) 
	{
		LOG_ERROR("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_Device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) 
	{
		LOG_ERROR("failed to allocate image memory!");
	}

	if (vkBindImageMemory(m_Device, image, imageMemory, 0) != VK_SUCCESS) {
		LOG_ERROR("failed to bind image memory!");
	}
}

void VulkanDevice::InitImguiForVulkan(ImGui_ImplVulkan_InitInfo& info)
{
	info.Instance = m_Instance;
	info.PhysicalDevice = m_PhysicalDevice;
	info.Device = m_Device;
	info.Queue = m_GraphicsQueue;
}

void VulkanDevice::SetObjectName(uint64_t object, VkDebugReportObjectTypeEXT objectType, const char* name)
{
	// Check for a valid function pointer
	if (pfnDebugMarkerSetObjectName)
	{
		VkDebugMarkerObjectNameInfoEXT nameInfo = {};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = objectType;
		nameInfo.object = object;
		nameInfo.pObjectName = name;
		pfnDebugMarkerSetObjectName(m_Device, &nameInfo);
	}
}

void VulkanDevice::BeginLabel(VkCommandBuffer commandBuffer, const char* name)
{
#ifdef _DEBUG
	glm::vec4 color = GetNextColor();
	VkDebugUtilsLabelEXT labelInfo{};
	labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	labelInfo.color[0] = color[0];
	labelInfo.color[1] = color[1];
	labelInfo.color[2] = color[2];
	labelInfo.color[3] = color[3];
	labelInfo.pLabelName = name;

	pfnCmdBeginDebugUtilsLabelEXT(commandBuffer, &labelInfo);
#endif
}

void VulkanDevice::EndLabel(VkCommandBuffer commandBuffer)
{
#ifdef _DEBUG
	pfnCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
}

VkBufferUsageFlags GetVkBufferUsageFlags(const BufferUsageFlags usageFlags)
{
	VkBufferUsageFlags bufferUsageFlags = 0;
	if (IsFlagSet(usageFlags & BufferUsageFlags::TransferSrc))
	{
		bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}

	if (IsFlagSet(usageFlags & BufferUsageFlags::TransferDst))
	{
		bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}

	if (IsFlagSet(usageFlags & BufferUsageFlags::ResrcBuffer))
	{
		bufferUsageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		bufferUsageFlags |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
	}

	if (IsFlagSet(usageFlags & BufferUsageFlags::StorageBuffer))
	{
		bufferUsageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		bufferUsageFlags |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
	}

	if (IsFlagSet(usageFlags & BufferUsageFlags::VertexBuffer))
	{
		bufferUsageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	}

	if (IsFlagSet(usageFlags & BufferUsageFlags::IndexBuffer))
	{
		bufferUsageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	}

	if (IsFlagSet(usageFlags & BufferUsageFlags::UniformBuffer))
	{
		bufferUsageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	}

	if (IsFlagSet(usageFlags & BufferUsageFlags::IndirectBuffer))
	{
		bufferUsageFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	}

	return bufferUsageFlags;
}

VmaMemoryUsage GetVmaMemoryUsageFrom(const MemoryAccess memoryAccess)
{
	switch (memoryAccess)
	{
	case MemoryAccess::Host:
		return VMA_MEMORY_USAGE_CPU_ONLY;
	case MemoryAccess::Device:
		return VMA_MEMORY_USAGE_GPU_ONLY;
	default:
		ASSERT(false, "Not supported MemoryAccessFlags.");
		return VMA_MEMORY_USAGE_UNKNOWN;
	}
}

VkDescriptorType GetVkDescriptorTypeFrom(const DescriptorType descriptorSetBinding)
{
	switch (descriptorSetBinding)
	{
	case DescriptorType::CombinedSampler:
		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	case DescriptorType::UniformBuffer:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	default:
		ASSERT(false, "Not supported DescriptorSetBindingType.");
		return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
}

VkShaderStageFlagBits GetVkShaderStageFrom(const ShaderType shaderType)
{
	switch (shaderType)
	{
	case ShaderType::Vertex:
		return VK_SHADER_STAGE_VERTEX_BIT;
	case ShaderType::Fragment:
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	case ShaderType::Geometry:
		return VK_SHADER_STAGE_GEOMETRY_BIT;
	case ShaderType::Hull:
		return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	case ShaderType::Domain:
		return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	default:
		ASSERT(false, "Not supported ShaderType.");
		return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
	}
}

VkDescriptorType GetVkDescriptorTypeFrom(const SpvReflectDescriptorType reflectDescriptorType)
{
	switch (reflectDescriptorType)
	{
	case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	default:
		ASSERT(false, "Not supported SpvReflectDescriptorBinding.");
		return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
}

VkFormat GetVkFormatFrom(const Format format)
{
	switch (format)
	{
	case Format::D32_SFLOAT:
		return VK_FORMAT_D32_SFLOAT;
	case Format::D32_SFLOAT_S8_UINT:
		return VK_FORMAT_D32_SFLOAT_S8_UINT;
	case Format::B8G8R8A8_UNORM:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case Format::B8G8R8A8_UNORM_SRGB:
		return VK_FORMAT_B8G8R8A8_SRGB;
	case Format::R8G8B8A8_UNORM:
		return VK_FORMAT_R8G8B8A8_UNORM;
	case Format::R8G8B8A8_UNORM_SRGB:
		return VK_FORMAT_R8G8B8A8_SRGB;
	case Format::R32_SFLOAT:
		return VK_FORMAT_R32_SFLOAT;
	case Format::R8_UNORM:
		return VK_FORMAT_R8_UNORM;
	case Format::R16G16B16A16_FLOAT:
		return VK_FORMAT_R16G16B16A16_SFLOAT;
	case Format::R32G32B32A32_FLOAT:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	case Format::R32G32B32_FLOAT:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case Format::R32G32_FLOAT:
		return VK_FORMAT_R32G32_SFLOAT;
	case Format::BC1_UNORM:
		return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
	case Format::BC1_UNORM_SRGB:
		return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
	case Format::BC2_UNORM:
		return VK_FORMAT_BC2_UNORM_BLOCK;
	case Format::BC2_UNORM_SRGB:
		return VK_FORMAT_BC2_SRGB_BLOCK;
	case Format::BC3_UNORM:
		return VK_FORMAT_BC3_UNORM_BLOCK;
	case Format::BC3_UNORM_SRGB:
		return VK_FORMAT_BC3_SRGB_BLOCK;
	case Format::BC7_UNORM:
		return VK_FORMAT_BC7_UNORM_BLOCK;
	case Format::BC7_UNORM_SRGB:
		return VK_FORMAT_BC7_SRGB_BLOCK;
	case Format::R32_UINT:
		return VK_FORMAT_R32_UINT;
	default:
		ASSERT(false, "Not supported Format.");
		return VK_FORMAT_UNDEFINED;
	}
}

VkColorSpaceKHR GetVkColorSpaceFrom(const ColorSpace colorSpace)
{
	switch (colorSpace)
	{
	case ColorSpace::SRGB:
		return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	case ColorSpace::ExtendedSRGB_Linear:
		return VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT;
	case ColorSpace::ExtendedSRGB_Nonlinear:
		return VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT;
	default:
		ASSERT(false, "Not supported ColorSpace.");
		return VK_COLOR_SPACE_MAX_ENUM_KHR;
	}
}

uint32_t GetBlockSizeFrom(const VkFormat format)
{
	if ((format == VK_FORMAT_BC1_RGBA_UNORM_BLOCK) ||
		(format == VK_FORMAT_BC1_RGBA_SRGB_BLOCK) ||
		(format == VK_FORMAT_BC2_UNORM_BLOCK) ||
		(format == VK_FORMAT_BC2_SRGB_BLOCK) ||
		(format == VK_FORMAT_BC3_UNORM_BLOCK) ||
		(format == VK_FORMAT_BC3_SRGB_BLOCK) ||
		(format == VK_FORMAT_BC4_UNORM_BLOCK) ||
		(format == VK_FORMAT_BC5_UNORM_BLOCK) ||
		(format == VK_FORMAT_BC6H_UFLOAT_BLOCK) ||
		(format == VK_FORMAT_BC7_UNORM_BLOCK) ||
		(format == VK_FORMAT_BC7_SRGB_BLOCK))
	{
		return 4;
	}
	else
	{
		return 1;
	}
}

VkImageUsageFlags GetVkImageUsageFlagsFrom(const ImageUsageFlags usageFlags)
{
	VkImageUsageFlags imageUsageFlags = 0;
	if (IsFlagSet(usageFlags & ImageUsageFlags::TransferSrc))
	{
		imageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	if (IsFlagSet(usageFlags & ImageUsageFlags::TransferDst))
	{
		imageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	if (IsFlagSet(usageFlags & ImageUsageFlags::Resource))
	{
		imageUsageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	}

	if (IsFlagSet(usageFlags & ImageUsageFlags::Storage))
	{
		imageUsageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
	}

	if (IsFlagSet(usageFlags & ImageUsageFlags::RenderTarget))
	{
		imageUsageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	if (IsFlagSet(usageFlags & ImageUsageFlags::DepthStencil))
	{
		imageUsageFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}

	return imageUsageFlags;
}

VkImageType GetVkImageTypeFrom(const uint32_t imageDepth)
{
	if (imageDepth > 1)
	{
		return VK_IMAGE_TYPE_3D;
	}
	else
	{
		return VK_IMAGE_TYPE_2D;
	}
}

VkImageAspectFlags GetVkImageAspectFlagsFrom(VkFormat format)
{
	switch (format)
	{
	case VK_FORMAT_D16_UNORM:
	case VK_FORMAT_X8_D24_UNORM_PACK32:
	case VK_FORMAT_D32_SFLOAT:
		return VK_IMAGE_ASPECT_DEPTH_BIT;
	case VK_FORMAT_S8_UINT:
		return VK_IMAGE_ASPECT_STENCIL_BIT;
	case VK_FORMAT_D16_UNORM_S8_UINT:
	case VK_FORMAT_D24_UNORM_S8_UINT:
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	default:
		return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}

VkImageLayout GetVkImageLayoutFrom(const ResourceState resourceState)
{
	if (resourceState == ResourceState::None)
	{
		return VK_IMAGE_LAYOUT_UNDEFINED;
	}
	else if (resourceState == ResourceState::TransferSrc)
	{
		return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	}
	else if (resourceState == ResourceState::TransferDst)
	{
		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	}
	else if (resourceState == ResourceState::DepthStencilRead)
	{
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	}
	else if (resourceState == ResourceState::DepthStencilWrite)
	{
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}
	else if (resourceState == ResourceState::GenericRead)
	{
		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
	else if (resourceState == ResourceState::RenderTarget)
	{
		return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	else if (resourceState == ResourceState::Present)
	{
		return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}

	ASSERT(false, "Not supported ResourceState.");
	return VK_IMAGE_LAYOUT_UNDEFINED;
}

VkAccessFlags GetVkAccessFlagsFrom(const ResourceState resourceState)
{
	if (resourceState == ResourceState::None)
	{
		return VkAccessFlagBits(0);
	}
	else if (resourceState == ResourceState::TransferSrc)
	{
		return VK_ACCESS_TRANSFER_READ_BIT;
	}
	else if (resourceState == ResourceState::TransferDst)
	{
		return VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (resourceState == ResourceState::DepthStencilRead)
	{
		return VkAccessFlagBits(VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT);
	}
	else if (resourceState == ResourceState::DepthStencilWrite)
	{
		return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else if (resourceState == ResourceState::GenericRead)
	{
		return VkAccessFlagBits(VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT |
			VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT |
			VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT);
	}
	else if (resourceState == ResourceState::RenderTarget)
	{
		return VkAccessFlagBits(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
	}
	else if (resourceState == ResourceState::Present)
	{
		return VK_ACCESS_MEMORY_READ_BIT;
	}

	ASSERT(false, "Not supported ResourceState.");
	return VK_IMAGE_LAYOUT_UNDEFINED;
}

VkPipelineStageFlagBits GetGraphicsPipelineStage(const ResourceState resourceState)
{
	switch (resourceState)
	{
	case ResourceState::TransferSrc:
	case ResourceState::TransferDst:
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	case ResourceState::DepthStencilRead:
		return VkPipelineStageFlagBits(
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
			VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
			VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
	case ResourceState::DepthStencilWrite:
		return VkPipelineStageFlagBits(
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
			VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
	case ResourceState::GenericRead:
		return VkPipelineStageFlagBits(
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
			VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
			VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
			VK_PIPELINE_STAGE_TRANSFER_BIT);
	case ResourceState::RenderTarget:
		return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	case ResourceState::None:
	case ResourceState::Present:
		return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	default:
		ASSERT(false, "Not supported ResourceState.");
	}

	return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
}

VkPipelineStageFlagBits GetTransferPipelineStage(const ResourceState resourceState)
{
	if ((resourceState == ResourceState::TransferSrc) || (resourceState == ResourceState::TransferDst))
	{
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	}

	return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
}


VkPrimitiveTopology GetVkPrimitiveTopologyFrom(const Topology topology)
{
	switch (topology)
	{
	case Topology::PointList:
		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case Topology::LineList:
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case Topology::LineStrip:
		return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	case Topology::TriangleList:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	case Topology::TriangleStrip:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	case Topology::PatchList:
		return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	default:
		ASSERT(false, "Not supported Topology.");
		return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
	}
}

VkSampleCountFlagBits GetVkSampleFlagsFrom(const MultisampleType multiSampleType)
{
	switch (multiSampleType)
	{
	case MultisampleType::Sample_1:
		return VK_SAMPLE_COUNT_1_BIT;
	case MultisampleType::Sample_2:
		return VK_SAMPLE_COUNT_2_BIT;
	case MultisampleType::Sample_4:
		return VK_SAMPLE_COUNT_4_BIT;
	case MultisampleType::Sample_8:
		return VK_SAMPLE_COUNT_8_BIT;
	default:
		ASSERT(false, "Not supported MultisampleType.");
		return VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
	}
}

VkCompareOp GetVkCompareOperationFrom(const CompareOperation compareOperation)
{
	switch (compareOperation)
	{
	case CompareOperation::Never:
		return VK_COMPARE_OP_NEVER;
	case CompareOperation::Less:
		return VK_COMPARE_OP_LESS;
	case CompareOperation::Equal:
		return VK_COMPARE_OP_EQUAL;
	case CompareOperation::LessEqual:
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	case CompareOperation::Greater:
		return VK_COMPARE_OP_GREATER;
	case CompareOperation::NotEqual:
		return VK_COMPARE_OP_NOT_EQUAL;
	case CompareOperation::GreaterEqual:
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case CompareOperation::Always:
		return VK_COMPARE_OP_ALWAYS;
	default:
		ASSERT(false, "Not supported CompareOperation.");
		return VK_COMPARE_OP_MAX_ENUM;
	}
}

VkStencilOp GetVkStencilOpFrom(const StencilOperation stencilOperation)
{
	switch (stencilOperation)
	{
	case StencilOperation::Keep:
		return VK_STENCIL_OP_KEEP;
	case StencilOperation::Zero:
		return VK_STENCIL_OP_ZERO;
	case StencilOperation::Replace:
		return VK_STENCIL_OP_REPLACE;
	case StencilOperation::IncrementAndClamp:
		return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
	case StencilOperation::DecrementAndClamp:
		return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
	case StencilOperation::Invert:
		return VK_STENCIL_OP_INVERT;
	case StencilOperation::Increment:
		return VK_STENCIL_OP_INCREMENT_AND_WRAP;
	case StencilOperation::Decrement:
		return VK_STENCIL_OP_DECREMENT_AND_WRAP;
	default:
		ASSERT(false, "Not supported StencilOperation.");
		return VK_STENCIL_OP_MAX_ENUM;
	}
}

VkFilter GetVkFilterFrom(const FilterType filterType)
{
	switch (filterType)
	{
	case FilterType::Point:
		return VK_FILTER_NEAREST;
	case FilterType::Linear:
		return VK_FILTER_LINEAR;
	case FilterType::Anisotropic:
		return VK_FILTER_LINEAR;
	default:
		ASSERT(false, "Not supported FilterType.");
		return VK_FILTER_MAX_ENUM;
	}
}

VkSamplerMipmapMode GetVkMipmapModeFrom(const FilterType filterType)
{
	switch (filterType)
	{
	case FilterType::Point:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	case FilterType::Linear:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	default:
		ASSERT(false, "Not supported FilterType.");
		return VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;
	}
}

VkSamplerAddressMode GetVkAddressModeFrom(const AddressMode addressMode)
{
	switch (addressMode)
	{
	case AddressMode::Repeat:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case AddressMode::Mirror:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case AddressMode::Clamp:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case AddressMode::Border:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	case AddressMode::MirrorClamp:
		return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	default:
		ASSERT(false, "Not supported AddressMode.");
		return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
	}
}

VkBorderColor GetVkBorderColorFrom(const Color color)
{
	if (color.value[0] == 0.0f && color.value[1] == 0.0f && color.value[2] == 0.0f)
	{
		return color.value[3] == 1.0f ? VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK : VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	}
	else if (color.value[0] == 1.0f && color.value[1] == 1.0f && color.value[2] == 1.0f && color.value[3] == 1.0f)
	{
		return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	}
	else
	{
		ASSERT(false, "Not supported border Color.");
		return VK_BORDER_COLOR_MAX_ENUM;
	}
}

VkBlendFactor GetVkBlendFactorFrom(const BlendValue blendValue)
{
	switch (blendValue)
	{
	case BlendValue::Zero:
		return VK_BLEND_FACTOR_ZERO;
	case BlendValue::One:
		return VK_BLEND_FACTOR_ONE;
	case BlendValue::SrcColor:
		return VK_BLEND_FACTOR_SRC_COLOR;
	case BlendValue::InvSrcColor:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	case BlendValue::SrcAlpha:
		return VK_BLEND_FACTOR_SRC_ALPHA;
	case BlendValue::InvSrcAlpha:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case BlendValue::DestAlpha:
		return VK_BLEND_FACTOR_DST_ALPHA;
	case BlendValue::InvDestAlpha:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	case BlendValue::DstColor:
		return VK_BLEND_FACTOR_DST_COLOR;
	case BlendValue::InvDstColor:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	case BlendValue::BlendFactor:
		return VK_BLEND_FACTOR_CONSTANT_COLOR;
	case BlendValue::InvBlendFactor:
		return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
	case BlendValue::SrcAlphaSat:
		return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
	case BlendValue::Src1Color:
		return VK_BLEND_FACTOR_SRC1_COLOR;
	case BlendValue::InvSrc1Color:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
	case BlendValue::Src1Alpha:
		return VK_BLEND_FACTOR_SRC1_ALPHA;
	case BlendValue::InvSrc1Alpha:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
	default:
		ASSERT(false, "Not supported BlendValue.");
		return VK_BLEND_FACTOR_MAX_ENUM;
	}
}

VkBlendOp GetVkBlendOpFrom(const BlendOperation blendOperation)
{
	switch (blendOperation)
	{
	case BlendOperation::Add:
		return VK_BLEND_OP_ADD;
	case BlendOperation::Subtract:
		return VK_BLEND_OP_SUBTRACT;
	case BlendOperation::ReverseSubtract:
		return VK_BLEND_OP_REVERSE_SUBTRACT;
	case BlendOperation::Min:
		return VK_BLEND_OP_MIN;
	case BlendOperation::Max:
		return VK_BLEND_OP_MAX;
	default:
		ASSERT(false, "Not supported BlendOperation.");
		return VK_BLEND_OP_MAX_ENUM;
	}
}

VkVertexInputRate GetVkVertexInputRateFrom(const VertexInputRate inputRate)
{
	switch (inputRate)
	{
	case VertexInputRate::PerVertex:
		return VK_VERTEX_INPUT_RATE_VERTEX;
	case VertexInputRate::PerInstance:
		return VK_VERTEX_INPUT_RATE_INSTANCE;
	default:
		ASSERT(false, "Not supported VertexInputRate.");
		return VK_VERTEX_INPUT_RATE_MAX_ENUM;
	}
}

VkClearValue GetVkClearColorValueForFormat(const VkFormat format)
{
	switch (format)
	{
	case VK_FORMAT_R8G8B8A8_SRGB:
	case VK_FORMAT_B8G8R8A8_SRGB:
	case VK_FORMAT_B8G8R8A8_UNORM:
	case VK_FORMAT_R8G8B8A8_UNORM:
	case VK_FORMAT_R16G16B16A16_SFLOAT:
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		return VkClearValue{ 0.5f, 0.9f, 1.f, 1.0f };
	case VK_FORMAT_R8_UNORM:
	case VK_FORMAT_R32_SFLOAT:
		return VkClearValue{ 0.1f };
	case VK_FORMAT_R32G32B32_SFLOAT:
		return VkClearValue{ 0.1f, 0.1f, 0.1f};
	case VK_FORMAT_R32G32_SFLOAT:
		return VkClearValue{ 0.1f, 0.1f};
	case VK_FORMAT_R32_UINT:
		return VkClearValue{ 0 };
	default:
		ASSERT(false, "Not supported format.");
		break;
	}
}
