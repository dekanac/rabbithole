#pragma once

#include <vector>

#include "Render/Vulkan/VulkanRenderPass.h"
#include "Render/Vulkan/VulkanFramebuffer.h"
#include "Render/Vulkan/VulkanTypes.h"

class VulkanDevice;
class VulkanImageView;
class VulkanCommandBuffer;

struct RenderPassInfo
{
	bool ClearRenderTargets = true;
	bool ClearDepth = true;
	bool ClearStencil = true;
	ResourceState InitialRenderTargetState = ResourceState::None;
	ResourceState FinalRenderTargetState = ResourceState::RenderTarget;
	ResourceState InitialDepthStencilState = ResourceState::DepthStencilWrite;
	ResourceState FinalDepthStencilState = ResourceState::DepthStencilWrite;
	Extent2D extent = { 0, 0 };
};

class RenderPass
{
public:
	RenderPass(VulkanDevice& device,
		const std::vector<VulkanImageView*> renderTargetViews,
		const VulkanImageView* depthStencilView,
		const RenderPassInfo& info,
		const char* name);
	~RenderPass();

	static void DefaultRenderPassInfo(RenderPassInfo& info, uint32_t width, uint32_t height);

	void BeginRenderPass(VulkanCommandBuffer& commandBuffer);
	void EndRenderPass(VulkanCommandBuffer& commandBuffer);

	VulkanRenderPass&	GetVulkanRenderPass() { return *m_RenderPass; }
	VulkanFramebuffer&	GetVulkanFramebuffer() { return *m_Framebuffer; }

private:
	VulkanDevice&		m_Device;
	VulkanRenderPass*	m_RenderPass;
	VulkanFramebuffer*	m_Framebuffer;

	Extent2D						m_Extent;
	uint32_t						m_RTCount = 0;
	bool							m_HasDepth = false;
	std::vector<VkClearValue>		m_ClearValues;
};