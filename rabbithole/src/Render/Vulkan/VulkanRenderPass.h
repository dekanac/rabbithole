#pragma once

class VulkanDevice;
class VulkanImageView;

struct RenderPassConfigInfo
{
	bool ClearRenderTargets;
	bool ClearDepth;
	bool ClearStencil;
	ResourceState InitialRenderTargetState;
	ResourceState FinalRenderTargetState;
    ResourceState InitialDepthStencilState;
    ResourceState FinalDepthStencilState;
};

class VulkanRenderPass
{
public:
	VulkanRenderPass(const VulkanDevice* device, const std::vector<VulkanImageView*> renderTargetViews,
		const VulkanImageView* depthStencilView, const RenderPassConfigInfo& info, const char* name);
	~VulkanRenderPass();

	VkRenderPass GetRenderPass() const { return m_RenderPass; }
    static void DefaultRenderPassInfo(RenderPassConfigInfo*& renderPassInfo);

private:
	VkRenderPass				m_RenderPass;
	
    const VulkanDevice*			m_VulkanDevice;
	const RenderPassConfigInfo	m_Info;

	uint8_t						m_AttachmentCount;
	VkClearValue				m_ClearValues[MaxRenderTargetCount];

	
};