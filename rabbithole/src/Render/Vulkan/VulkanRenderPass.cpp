#include "precomp.h"

VulkanRenderPass::VulkanRenderPass(
	const VulkanDevice* device, 
	const std::vector<VulkanImageView*> renderTargetViews, 
	const VulkanImageView* depthStencilView, 
	const VulkanRenderPassInfo& info, 
	const char* name)
	: m_VulkanDevice(device)
	, m_Info(info)
{
	uint32_t colorAttachmentCount = static_cast<uint8_t>(renderTargetViews.size());
	m_AttachmentCount = colorAttachmentCount;

	std::vector<VkAttachmentDescription> attachmentDescriptions;
	std::vector<VkAttachmentReference> colorAttachmentReferences;
	for (uint32_t i = 0; i < colorAttachmentCount; ++i)
	{
		VulkanImageView* renderTargetView = renderTargetViews[i];

		ClearValue clearColor = renderTargetView->GetInfo().ClearValue;
		VkClearValue clearValue{};
		clearValue.color.float32[0] = clearColor.Color.value[0];
		clearValue.color.float32[1] = clearColor.Color.value[1];
		clearValue.color.float32[2] = clearColor.Color.value[2];
		clearValue.color.float32[3] = clearColor.Color.value[3];
		m_ClearValues[i] = clearValue;

		VkAttachmentDescription attachmentDescription{};
		attachmentDescription.format = renderTargetView->GetFormat();
		attachmentDescription.samples = GetVkSampleFlagsFrom(renderTargetView->GetInfo().Resource->GetInfo().MultisampleType);
		attachmentDescription.loadOp = m_Info.ClearRenderTargets ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescription.initialLayout = GetVkImageLayoutFrom(m_Info.InitialRenderTargetState);
		attachmentDescription.finalLayout = GetVkImageLayoutFrom(m_Info.FinalRenderTargetState);
		attachmentDescriptions.push_back(attachmentDescription);

		VkAttachmentReference attachmentReference{};
		attachmentReference.attachment = static_cast<uint32_t>(attachmentDescriptions.size() - 1);
		attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachmentReferences.push_back(attachmentReference);
	}

	VkAttachmentReference depthStencilAttachmentReference;
	if (depthStencilView)
	{
		m_AttachmentCount++;

		ClearValue clearDepthStencil = depthStencilView->GetInfo().ClearValue;
		VkClearValue depthStencil{};
		depthStencil.depthStencil.depth = clearDepthStencil.DepthStencil.Depth;
		depthStencil.depthStencil.stencil = clearDepthStencil.DepthStencil.Stencil;
		m_ClearValues[colorAttachmentCount] = depthStencil;

		bool readDepthOrStencil = IsFlagSet(depthStencilView->GetInfo().Flags & ImageViewFlags::ReadDepth) ||
			IsFlagSet(depthStencilView->GetInfo().Flags & ImageViewFlags::ReadStencil);
		ResourceState depthStencilState = readDepthOrStencil ? ResourceState::DepthStencilRead : ResourceState::DepthStencilWrite;
		VkImageLayout depthStencilLayout = GetVkImageLayoutFrom(depthStencilState);

		VkAttachmentDescription attachmentDescription{};
		attachmentDescription.format = depthStencilView->GetFormat();
		attachmentDescription.samples = GetVkSampleFlagsFrom(depthStencilView->GetInfo().Resource->GetInfo().MultisampleType);
		attachmentDescription.loadOp = m_Info.ClearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescription.stencilLoadOp = m_Info.ClearStencil ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescription.initialLayout = depthStencilLayout;
		attachmentDescription.finalLayout = depthStencilLayout;
		attachmentDescriptions.push_back(attachmentDescription);

		depthStencilAttachmentReference.attachment = static_cast<uint32_t>(attachmentDescriptions.size() - 1);
		depthStencilAttachmentReference.layout = depthStencilLayout;
	}

	VkSubpassDescription subpassDescription{};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = colorAttachmentCount;
	subpassDescription.pColorAttachments = colorAttachmentCount ? colorAttachmentReferences.data() : nullptr;
	subpassDescription.pDepthStencilAttachment = depthStencilView ? &depthStencilAttachmentReference : nullptr;

	VkRenderPassCreateInfo renderPassCreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
	renderPassCreateInfo.pAttachments = attachmentDescriptions.data();
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.subpassCount = 1;

	VULKAN_API_CALL(vkCreateRenderPass(m_VulkanDevice->GetGraphicDevice(), &renderPassCreateInfo, nullptr, &m_RenderPass));
}

VulkanRenderPass::~VulkanRenderPass()
{
	vkDestroyRenderPass(m_VulkanDevice->GetGraphicDevice(), m_RenderPass, nullptr);
}
