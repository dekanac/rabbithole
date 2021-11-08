#pragma once

class VulkanDevice;

struct VulkanFramebufferInfo
{
	uint32_t width;
	uint32_t height;
};

class VulkanFramebuffer
{
public:
	VulkanFramebuffer(const VulkanDevice* device, 
		const VulkanFramebufferInfo& info,
		const VulkanRenderPass* renderPass,
		const std::vector<VulkanImageView*> attachments, 
		const VulkanImageView* depthStencil);
	~VulkanFramebuffer();

	VkFramebuffer			GetVkFramebuffer() const { return m_Framebuffer; }
	VulkanFramebufferInfo	GetInfo() const { return m_Info; }

private:
	const VulkanDevice*				m_VulkanDevice;
	const VulkanFramebufferInfo&	m_Info;

	VkFramebuffer					m_Framebuffer;


};