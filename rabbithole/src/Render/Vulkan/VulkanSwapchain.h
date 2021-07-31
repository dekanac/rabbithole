#pragma once

#include "VulkanDevice.h"

// vulkan headers
#include <vulkan/vulkan.h>

// std lib headers
#include <string>
#include <vector>

class VulkanImage;
class VulkanImageView;

class VulkanSwapchain {
public:
	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	VulkanSwapchain(VulkanDevice& deviceRef, VkExtent2D windowExtent);
	~VulkanSwapchain();

	VulkanSwapchain(const VulkanSwapchain&) = delete;
	VulkanSwapchain operator=(const VulkanSwapchain&) = delete;

	VkFramebuffer	const GetFrameBuffer(int index) const { return m_SwapChainFramebuffers[index]; }
	VkRenderPass	const GetRenderPass() const { return m_RenderPass; }
	VkImageView		const GetImageView(int index) const { return m_SwapChainImageViews[index]; }
	size_t			const GetImageCount() const { return m_SwapChainImages.size(); }
	VkFormat		const GetSwapChainImageFormat() const { return m_SwapChainImageFormat; }
	VkExtent2D		const GetSwapChainExtent() const { return m_SwapChainExtent; }
	uint32_t		const GetWidth() const { return m_SwapChainExtent.width; }
	uint32_t		const GetHeight() const { return m_SwapChainExtent.height; }

	float			ExtentAspectRatio() { return static_cast<float>(m_SwapChainExtent.width) / static_cast<float>(m_SwapChainExtent.height); }
	VkFormat		FindDepthFormat();

	VkResult		AcquireNextImage(uint32_t* imageIndex);
	VkResult		SubmitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

private:
	void CreateSwapChain();
	void CreateImageViews();
	void CreateDepthResources();
	void CreateRenderPass();
	void CreateFramebuffers();
	void CreateSyncObjects();

	// Helper functions
	VkSurfaceFormatKHR	ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR	ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D			ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	VulkanDevice&					m_VulkanDevice;
	VkSwapchainKHR					m_SwapChain;

	VkFormat						m_SwapChainImageFormat;
	VkExtent2D						m_SwapChainExtent;
	VkExtent2D						m_WindowExtent;

	std::vector<VkFramebuffer>		m_SwapChainFramebuffers;
	VkRenderPass					m_RenderPass;

	std::vector<VulkanImage*>		m_DepthImages;
	std::vector<VulkanImageView*>	m_DepthImageViews;
	std::vector<VkImage>			m_SwapChainImages;
	std::vector<VkImageView>		m_SwapChainImageViews;

	std::vector<VkSemaphore>		m_ImageAvailableSemaphores;
	std::vector<VkSemaphore>		m_RenderFinishedSemaphores;
	std::vector<VkFence>			m_InFlightFences;
	std::vector<VkFence>			m_ImagesInFlight;
	size_t							m_CurrentFrame = 0;
};