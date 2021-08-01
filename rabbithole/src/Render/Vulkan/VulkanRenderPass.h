#pragma once

class VulkanDevice;
class VulkanImageView;

struct VulkanRenderPassInfo
{
	bool ClearRenderTargets;
	bool ClearDepth;
	bool ClearStencil;
	ResourceState InitialRenderTargetState;
	ResourceState FinalRenderTargetState;
};

class VulkanRenderPass
{
public:
	VulkanRenderPass(const VulkanDevice* device, const std::vector<VulkanImageView*> renderTargetViews,
		const VulkanImageView* depthStencilView, const VulkanRenderPassInfo& info, const char* name);
	~VulkanRenderPass();

	VkRenderPass GetRenderPass() const { return m_RenderPass; }

private:
	VkRenderPass				m_RenderPass;
	
	const VulkanDevice*			m_VulkanDevice;
	const VulkanRenderPassInfo	m_Info;

	uint8_t						m_AttachmentCount;
	VkClearValue				m_ClearValues[MaxRenderTargetCount];

	
};