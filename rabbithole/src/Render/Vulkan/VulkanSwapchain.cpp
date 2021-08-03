#include "precomp.h"

// std
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

VulkanSwapchain::VulkanSwapchain(VulkanDevice& deviceRef, VkExtent2D extent)
	: m_VulkanDevice{ deviceRef }, m_WindowExtent{ extent } 
{
	CreateSwapChain();
	CreateImageViews();
	CreateDepthResources();
	CreateRenderPass();
	CreateFramebuffers();
	CreateSyncObjects();
}

VulkanSwapchain::~VulkanSwapchain() 
{
	
	if (m_SwapChain != nullptr) 
	{
		vkDestroySwapchainKHR(m_VulkanDevice.GetGraphicDevice(), m_SwapChain, nullptr);
		m_SwapChain = nullptr;
	}

	delete(m_RenderPass);
	delete(m_DepthStencil);
	// cleanup synchronization objects
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		vkDestroySemaphore(m_VulkanDevice.GetGraphicDevice(), m_RenderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_VulkanDevice.GetGraphicDevice(), m_ImageAvailableSemaphores[i], nullptr);
		vkDestroyFence(m_VulkanDevice.GetGraphicDevice(), m_InFlightFences[i], nullptr);
	}
}

VkResult VulkanSwapchain::AcquireNextImage(uint32_t* imageIndex) 
{
	vkWaitForFences(
		m_VulkanDevice.GetGraphicDevice(),
		1,
		&m_InFlightFences[m_CurrentFrame],
		VK_TRUE,
		std::numeric_limits<uint64_t>::max());

	VkResult result = vkAcquireNextImageKHR(
		m_VulkanDevice.GetGraphicDevice(),
		m_SwapChain,
		std::numeric_limits<uint64_t>::max(),
		m_ImageAvailableSemaphores[m_CurrentFrame],  // must be a not signaled semaphore
		VK_NULL_HANDLE,
		imageIndex);

	return result;
}

VkResult VulkanSwapchain::SubmitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex) 
{
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

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = buffers;

	VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(m_VulkanDevice.GetGraphicDevice(), 1, &m_InFlightFences[m_CurrentFrame]);
	VULKAN_API_CALL(vkQueueSubmit(m_VulkanDevice.GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]), "failed to submit draw command buffer!");

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { m_SwapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = imageIndex;

	auto result = vkQueuePresentKHR(m_VulkanDevice.GetPresentQueue(), &presentInfo);

	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return result;
}

void VulkanSwapchain::CreateSwapChain() 
{
	SwapChainSupportDetails swapChainSupport = m_VulkanDevice.GetSwapChainSupport();

	Format surfaceFormat = Format::B8G8R8A8_UNORM_SRGB;
	ColorSpace surfaceColorSpace = ColorSpace::SRGB;
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
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
		createInfo.queueFamilyIndexCount = 0;      // Optional
		createInfo.pQueueFamilyIndices = nullptr;  // Optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VULKAN_API_CALL(vkCreateSwapchainKHR(m_VulkanDevice.GetGraphicDevice(), &createInfo, nullptr, &m_SwapChain), "failed to create swap chain!");

	// we only specified a minimum number of images in the swap chain, so the implementation is
	// allowed to create a swap chain with more. That's why we'll first query the final number of
	// images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
	// retrieve the handles.

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

void VulkanSwapchain::CreateRenderPass() 
{
	VulkanRenderPassInfo renderPassInfo{};

	renderPassInfo.ClearDepth = true;
	renderPassInfo.ClearRenderTargets = true;
	renderPassInfo.ClearStencil = true;
	renderPassInfo.InitialRenderTargetState = ResourceState::None;
	renderPassInfo.FinalRenderTargetState = ResourceState::Present;

	std::vector<VulkanImageView*> renderTargetViews;
	renderTargetViews.push_back(m_SwapChainVulkanImageViews[0]);

	m_RenderPass = new VulkanRenderPass(&m_VulkanDevice, renderTargetViews, m_DepthStencil->GetView(), renderPassInfo, "swapchain renderpass");
	
}

void VulkanSwapchain::CreateFramebuffers() 
{
	m_SwapChainFramebuffers.resize(GetImageCount());
	for (size_t i = 0; i < GetImageCount(); i++) 
	{
		VulkanFramebufferInfo framebufferInfo{};
		framebufferInfo.height = GetSwapChainExtent().height;
		framebufferInfo.width = GetSwapChainExtent().width;
			
		m_SwapChainFramebuffers[i] = new VulkanFramebuffer(&m_VulkanDevice, framebufferInfo, m_RenderPass, { m_SwapChainVulkanImageViews[i] }, m_DepthStencil->GetView());
	}
}

void VulkanSwapchain::CreateDepthResources() 
{
	m_DepthStencil = new VulkanTexture(&m_VulkanDevice, m_SwapChainExtent.width, m_SwapChainExtent.height ,TextureFlags::DepthStencil | TextureFlags::RenderTarget, Format::D32_SFLOAT, "swapchain depthstencil");
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
		if (vkCreateSemaphore(m_VulkanDevice.GetGraphicDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) !=
			VK_SUCCESS ||
			vkCreateSemaphore(m_VulkanDevice.GetGraphicDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) !=
			VK_SUCCESS ||
			vkCreateFence(m_VulkanDevice.GetGraphicDevice(), &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS) 
		{
			LOG_ERROR("failed to create synchronization objects for a frame!");
		}
	}
}

VkSurfaceFormatKHR VulkanSwapchain::ChooseSwapSurfaceFormat(
	const std::vector<VkSurfaceFormatKHR>& availableFormats) 
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
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			std::cout << "Present mode: Mailbox" << std::endl;
			return availablePresentMode;
		}
	}

	// for (const auto &availablePresentMode : availablePresentModes) {
	//   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
	//     std::cout << "Present mode: Immediate" << std::endl;
	//     return availablePresentMode;
	//   }
	// }

	std::cout << "Present mode: V-Sync" << std::endl;
	return VK_PRESENT_MODE_FIFO_KHR;
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
		actualExtent.width = std::max(
			capabilities.minImageExtent.width,
			std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(
			capabilities.minImageExtent.height,
			std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

VkFormat VulkanSwapchain::FindDepthFormat() 
{
	return m_VulkanDevice.FindSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}