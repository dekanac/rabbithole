#include "precomp.h"


VulkanFramebuffer::VulkanFramebuffer(const VulkanDevice* device,
	const VulkanFramebufferInfo& info, 
	const VulkanRenderPass* renderPass, 
	const std::vector<VulkanImageView*> attachments, 
	const VulkanImageView* depthStencil)
	: m_VulkanDevice(device)
	, m_Info(info)
{
	uint32_t attachmentCount = attachments.size();
	std::vector<VkImageView> att(attachmentCount);

	for (uint32_t i = 0; i < attachmentCount; i++)
	{
		att[i] = attachments[i]->GetImageView();
	}

	if (depthStencil)
	{
		att.push_back(depthStencil->GetImageView());
		attachmentCount++;
	}

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass->GetVkRenderPass();
	framebufferInfo.attachmentCount = attachmentCount;
	framebufferInfo.pAttachments = att.data();
	framebufferInfo.width = info.width;
	framebufferInfo.height = info.height;
	framebufferInfo.layers = 1;

	VULKAN_API_CALL(vkCreateFramebuffer(m_VulkanDevice->GetGraphicDevice(), &framebufferInfo, nullptr, &m_Framebuffer));

}

VulkanFramebuffer::~VulkanFramebuffer()
{
	vkDestroyFramebuffer(m_VulkanDevice->GetGraphicDevice(), m_Framebuffer, nullptr);
}
