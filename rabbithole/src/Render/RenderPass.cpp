#include "RenderPass.h"

#include "Render/Converters.h"
#include "Render/Vulkan/VulkanCommandBuffer.h"

RenderPass::RenderPass(VulkanDevice& device, const std::vector<VulkanImageView*> renderTargetViews, const VulkanImageView* depthStencilView, const RenderPassInfo& info, const char* name)
	: m_Device(device)
	, m_Extent({ info.extent })
{
	m_RenderPass = new VulkanRenderPass(&device, renderTargetViews, depthStencilView, VulkanRenderPassInfo{
			.ClearRenderTargets = info.ClearRenderTargets,
			.ClearDepth = info.ClearDepth,
			.ClearStencil = info.ClearStencil,
			.InitialRenderTargetState = info.InitialRenderTargetState,
			.FinalRenderTargetState = info.FinalRenderTargetState,
			.InitialDepthStencilState = info.InitialDepthStencilState,
			.FinalDepthStencilState = info.FinalDepthStencilState
		}, name);

	m_Framebuffer = new VulkanFramebuffer(&device, VulkanFramebufferInfo{
			.width = info.extent.width,
			.height = info.extent.height
		}, m_RenderPass, renderTargetViews, depthStencilView);

	m_RTCount = static_cast<uint32_t>(renderTargetViews.size());
	m_ClearValues.resize(m_RTCount);

	for (uint32_t i = 0; i < m_RTCount; i++)
	{
		Format format = renderTargetViews[i]->GetFormat();
		m_ClearValues[i] = GetVkClearColorValueFor(format);
	}

	if (depthStencilView)
	{
		m_RTCount++;
		Format format = depthStencilView->GetFormat();
		m_ClearValues.push_back(GetVkClearColorValueFor(format));
	}
}

void RenderPass::BeginRenderPass(VulkanCommandBuffer& commandBuffer)
{
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = GET_VK_HANDLE_PTR(m_RenderPass);
	renderPassInfo.framebuffer = GET_VK_HANDLE_PTR(m_Framebuffer);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_Extent;

	renderPassInfo.clearValueCount = static_cast<uint32_t>(m_RTCount);
	renderPassInfo.pClearValues = m_ClearValues.data();

	vkCmdBeginRenderPass(GET_VK_HANDLE(commandBuffer), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderPass::EndRenderPass(VulkanCommandBuffer& commandBuffer)
{
	vkCmdEndRenderPass(GET_VK_HANDLE(commandBuffer));
}

RenderPass::~RenderPass()
{
	delete(m_Framebuffer);
	delete(m_RenderPass);
}

void RenderPass::DefaultRenderPassInfo(RenderPassInfo& info, uint32_t width, uint32_t height)
{
	info.ClearRenderTargets = LoadOp::DontCare;
	info.ClearDepth = LoadOp::DontCare;
	info.ClearStencil = LoadOp::DontCare;
	info.InitialRenderTargetState = ResourceState::None;
	info.FinalRenderTargetState = ResourceState::RenderTarget;
	info.InitialDepthStencilState = ResourceState::DepthStencilWrite;
	info.FinalDepthStencilState = ResourceState::DepthStencilWrite;
	info.extent.width = width;
	info.extent.height = height;
}
