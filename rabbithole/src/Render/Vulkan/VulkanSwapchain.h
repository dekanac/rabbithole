#pragma once

#include "VulkanDevice.h"

// vulkan headers
#include <vulkan/vulkan.h>

// std lib headers
#include <string>
#include <vector>

class VulkanImage;
class VulkanImageView;
class VulkanRenderPass;
class VulkanFramebuffer;
class VulkanTexture;

class VulkanSwapchain {
public:
	VulkanSwapchain(VulkanDevice& deviceRef, VkExtent2D windowExtent);
	~VulkanSwapchain();

	VulkanSwapchain(const VulkanSwapchain&) = delete;
	VulkanSwapchain operator=(const VulkanSwapchain&) = delete;

	VulkanFramebuffer*	  GetFrameBuffer(int index) const { return m_SwapChainFramebuffers[index]; }
	VulkanRenderPass*	  GetRenderPass() const { return m_RenderPass; }
	VulkanImageView*	  GetImageView(int index) const { return m_SwapChainVulkanImageViews[index]; }
	uint32_t		const GetImageCount() const { return static_cast<uint32_t>(m_SwapChainVulkanImages.size()); }
	Format				  GetSwapChainImageFormat() const { return m_SwapChainImageFormat; }
	VkExtent2D		const GetSwapChainExtent() const { return m_SwapChainExtent; }
	uint32_t		const GetWidth() const { return m_SwapChainExtent.width; }
	uint32_t		const GetHeight() const { return m_SwapChainExtent.height; }
	VulkanImage*	      GetSwapChainImage(uint32_t imageIndex) const { return m_SwapChainVulkanImages[imageIndex]; }

	float			ExtentAspectRatio() { return static_cast<float>(m_SwapChainExtent.width) / static_cast<float>(m_SwapChainExtent.height); }
	VkFormat		FindDepthFormat();

	VkResult		AcquireNextImage(uint32_t* imageIndex);
	VkResult		SubmitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

private:
	void CreateSwapChain();
	void CreateImageViews();
	void CreateRenderPass();
	void CreateFramebuffers();
	void CreateSyncObjects();

	// Helper functions
	VkSurfaceFormatKHR	ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR	ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D			ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	VulkanDevice&					m_VulkanDevice;
	VkSwapchainKHR					m_SwapChain;

	Format							m_SwapChainImageFormat;
	VkExtent2D						m_SwapChainExtent;
	VkExtent2D						m_WindowExtent;

	std::vector<VulkanFramebuffer*>	m_SwapChainFramebuffers;
	VulkanRenderPass*				m_RenderPass;

	std::vector<VulkanImage*>		m_SwapChainVulkanImages;
	std::vector<VulkanImageView*>   m_SwapChainVulkanImageViews;

	std::vector<VkSemaphore>		m_ImageAvailableSemaphores;
	std::vector<VkSemaphore>		m_RenderFinishedSemaphores;
	std::vector<VkFence>			m_InFlightFences;
	std::vector<VkFence>			m_ImagesInFlight;

	size_t							m_CurrentFrame = 0;

protected:
	friend class VulkanImage;
	std::vector<VkImage>			m_SwapChainImages;

};