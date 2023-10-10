#include "precomp.h"

#include "Render/Converters.h"
#include "Render/SuperResolutionManager.h"
#include "Render/Window.h"

#include <optick/src/optick.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

VulkanSwapchain::VulkanSwapchain(VulkanDevice& deviceRef, VkExtent2D extent, VkSwapchainKHR oldSwapchain)
	: m_VulkanDevice{ deviceRef }, m_WindowExtent{ extent } 
{
	CreateSwapChain(oldSwapchain);
	CreateImageViews();
	CreateSyncObjects();
}

VulkanSwapchain::~VulkanSwapchain() 
{
	if (m_SwapChain != nullptr) 
	{
		vkDestroySwapchainKHR(m_VulkanDevice.GetGraphicDevice(), m_SwapChain, nullptr);
		m_SwapChain = nullptr;
	}

	// cleanup synchronization objects
	for (size_t i = 0; i < GetImageCount(); i++) 
	{
		vkDestroySemaphore(m_VulkanDevice.GetGraphicDevice(), m_RenderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_VulkanDevice.GetGraphicDevice(), m_ImageAvailableSemaphores[i], nullptr);
		vkDestroyFence(m_VulkanDevice.GetGraphicDevice(), m_InFlightFences[i], nullptr);
	}

	for (auto object : m_SwapChainVulkanImageViews) { delete (object); }
}

VkResult VulkanSwapchain::AcquireNextImage(uint32_t* imageIndex) 
{
	OPTICK_EVENT();

	vkWaitForFences(m_VulkanDevice.GetGraphicDevice(), 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

	VkResult result = vkAcquireNextImageKHR(
		m_VulkanDevice.GetGraphicDevice(),
		m_SwapChain,
		std::numeric_limits<uint64_t>::max(),
		m_ImageAvailableSemaphores[m_CurrentFrame],  // must be a not signaled semaphore
		VK_NULL_HANDLE,
		imageIndex);

	return result;
}

VkResult VulkanSwapchain::SubmitCommandBufferAndPresent(VulkanCommandBuffer& buffer, uint32_t* imageIndex)
{
	OPTICK_EVENT();

	if (m_ImagesInFlight[*imageIndex] != VK_NULL_HANDLE) 
	{
		vkWaitForFences(m_VulkanDevice.GetGraphicDevice(), 1, &m_ImagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
	}
	m_ImagesInFlight[*imageIndex] = m_InFlightFences[m_CurrentFrame];

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	VkCommandBuffer commandBuffer = GET_VK_HANDLE(buffer);
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(m_VulkanDevice.GetGraphicDevice(), 1, &m_InFlightFences[m_CurrentFrame]);
	VULKAN_API_CALL(vkQueueSubmit(m_VulkanDevice.GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]));

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { m_SwapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = imageIndex;

	OPTICK_GPU_FLIP(m_SwapChain);
	OPTICK_CATEGORY("Present", Optick::Category::Wait);

	auto result = vkQueuePresentKHR(m_VulkanDevice.GetPresentQueue(), &presentInfo);

	m_CurrentFrame = (m_CurrentFrame + 1) % GetImageCount();

	return result;
}

void VulkanSwapchain::CreateSwapChain(VkSwapchainKHR oldSwapChain)
{
	SwapChainSupportDetails swapChainSupport = m_VulkanDevice.GetSwapChainSupport();

	Format surfaceFormat = Format::B8G8R8A8_UNORM_SRGB;
	ColorSpace surfaceColorSpace = ColorSpace::SRGB;
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = MAX_FRAMES_IN_FLIGHT;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) 
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_VulkanDevice.GetPresentingSurface();

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = GetVkFormatFrom(surfaceFormat);
	createInfo.imageColorSpace = GetVkColorSpaceFrom(surfaceColorSpace);
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = m_VulkanDevice.FindPhysicalQueueFamilies();
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

	if (indices.graphicsFamily != indices.presentFamily) 
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = oldSwapChain;

	VULKAN_API_CALL(vkCreateSwapchainKHR(m_VulkanDevice.GetGraphicDevice(), &createInfo, nullptr, &m_SwapChain));

	vkGetSwapchainImagesKHR(m_VulkanDevice.GetGraphicDevice(), m_SwapChain, &imageCount, nullptr);
	m_SwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_VulkanDevice.GetGraphicDevice(), m_SwapChain, &imageCount, m_SwapChainImages.data());

	m_SwapChainVulkanImages.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; i++)
	{
		m_SwapChainVulkanImages[i] = new VulkanImage(&m_VulkanDevice, this, i);
	}

	m_SwapChainImageFormat = surfaceFormat;
	m_SwapChainExtent = extent;
}

void VulkanSwapchain::CreateImageViews() 
{
	m_SwapChainVulkanImageViews.resize(m_SwapChainVulkanImages.size());
	for (size_t i = 0; i < m_SwapChainVulkanImages.size(); i++) 
	{
		Color colorClearValues{};
		colorClearValues.value[0] = 0.f;
		colorClearValues.value[1] = 0.f;
		colorClearValues.value[2] = 0.f;
		colorClearValues.value[3] = 0.f;

		DepthStencil depthStencilClearValue{};
		depthStencilClearValue.Depth = 0.0f;
		depthStencilClearValue.Stencil = 0;

		ClearValue clearValues{};
		clearValues.Color = colorClearValues;
		clearValues.DepthStencil = depthStencilClearValue;

		VulkanImageViewInfo imageViewInfo{};
		imageViewInfo.ClearValue = clearValues;
		imageViewInfo.Flags = ImageViewFlags::Color;
		imageViewInfo.Format = GetSwapChainImageFormat();
		imageViewInfo.Resource = m_SwapChainVulkanImages[i];
		imageViewInfo.Subresource.ArraySize = 1;
		imageViewInfo.Subresource.ArraySlice = 0;
		imageViewInfo.Subresource.MipSize = 1;
		imageViewInfo.Subresource.MipSlice = 0;

		m_SwapChainVulkanImageViews[i] = new VulkanImageView(&m_VulkanDevice, imageViewInfo, "swapchain image views");
	}
}

void VulkanSwapchain::CreateSyncObjects() 
{
	m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	m_ImagesInFlight.resize(GetImageCount(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		VULKAN_API_CALL(vkCreateSemaphore(m_VulkanDevice.GetGraphicDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]));
		VULKAN_API_CALL(vkCreateSemaphore(m_VulkanDevice.GetGraphicDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]));
		VULKAN_API_CALL(vkCreateFence(m_VulkanDevice.GetGraphicDevice(), &fenceInfo, nullptr, &m_InFlightFences[i]));
	}
}

VkSurfaceFormatKHR VulkanSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) 
{
	for (const auto& availableFormat : availableFormats) 
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR VulkanSwapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) 
{
	if (Window::instance().GetVSyncEnabled())
	{
		LOG_INFO("Present mode: V - Sync");
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			LOG_INFO("Present mode: Mailbox");
			return availablePresentMode;
		}
	}

	for (const auto &availablePresentMode : availablePresentModes) 
	{
		if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) 
		{
			LOG_INFO("Present mode: Immediate");
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_IMMEDIATE_KHR;
}

VkExtent2D VulkanSwapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) 
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) 
	{
		return capabilities.currentExtent;
	}
	else 
	{
		VkExtent2D actualExtent = m_WindowExtent;
		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}