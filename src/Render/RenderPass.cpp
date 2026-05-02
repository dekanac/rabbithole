#include "RenderPass.h"

#include "Render/Converters.h"
#include "Render/Vulkan/VulkanCommandBuffer.h"
#include "Vulkan/VulkanImage.h"

RenderPass::RenderPass(VulkanDevice& device, const std::vector<VulkanImageView*> renderTargetViews,
                       const VulkanImageView* depthStencilView, const RenderPassInfo& info,
                       const char* name)
    : m_Device(device), m_RenderTargetViews(renderTargetViews),
      m_DepthStencilView(depthStencilView), m_Info(info)
{
    m_Extent = {info.extent.width, info.extent.height};
}

void RenderPass::BeginRenderPass(VulkanCommandBuffer& commandBuffer)
{
    for (auto* view : m_RenderTargetViews)
    {
        m_Device.ResourceBarrier(commandBuffer, view->GetInfo().Resource,
                                 m_Info.InitialRenderTargetState, ResourceState::RenderTarget,
                                 ResourceStage::Graphics, ResourceStage::Graphics);
    }

    if (m_DepthStencilView)
    {
        m_Device.ResourceBarrier(commandBuffer, m_DepthStencilView->GetInfo().Resource,
                                 m_Info.InitialDepthStencilState, ResourceState::DepthStencilWrite,
                                 ResourceStage::Graphics, ResourceStage::Graphics);
    }

    std::vector<VkRenderingAttachmentInfo> colorAttachments;
    for (auto* view : m_RenderTargetViews)
    {
        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = view->GetVkHandle();
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = GetVkLoadOpFrom(m_Info.ClearRenderTargets);
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        ClearValue clearColor = view->GetInfo().ClearValue;
        colorAttachment.clearValue.color.float32[0] = clearColor.Color.value[0];
        colorAttachment.clearValue.color.float32[1] = clearColor.Color.value[1];
        colorAttachment.clearValue.color.float32[2] = clearColor.Color.value[2];
        colorAttachment.clearValue.color.float32[3] = clearColor.Color.value[3];

        colorAttachments.push_back(colorAttachment);
    }

    VkRenderingAttachmentInfo depthAttachment{};
    if (m_DepthStencilView)
    {
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = m_DepthStencilView->GetVkHandle();
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = GetVkLoadOpFrom(m_Info.ClearDepth);
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        ClearValue clearDepthStencil = m_DepthStencilView->GetInfo().ClearValue;
        depthAttachment.clearValue.depthStencil.depth = clearDepthStencil.DepthStencil.Depth;
        depthAttachment.clearValue.depthStencil.stencil = clearDepthStencil.DepthStencil.Stencil;
    }

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = {{0, 0}, m_Extent};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
    renderingInfo.pColorAttachments = colorAttachments.data();
    renderingInfo.pDepthAttachment = m_DepthStencilView ? &depthAttachment : nullptr;
    renderingInfo.pStencilAttachment = nullptr; // TODO: support stencil

    vkCmdBeginRendering(GET_VK_HANDLE(commandBuffer), &renderingInfo);
}

void RenderPass::EndRenderPass(VulkanCommandBuffer& commandBuffer)
{
    vkCmdEndRendering(GET_VK_HANDLE(commandBuffer));

    for (auto* view : m_RenderTargetViews)
    {
        if (m_Info.FinalRenderTargetState != ResourceState::RenderTarget)
        {
            m_Device.ResourceBarrier(commandBuffer, view->GetInfo().Resource,
                                     ResourceState::RenderTarget, m_Info.FinalRenderTargetState,
                                     ResourceStage::Graphics, ResourceStage::Graphics);
        }
    }

    if (m_DepthStencilView)
    {
        if (m_Info.FinalDepthStencilState != ResourceState::DepthStencilWrite)
        {
            m_Device.ResourceBarrier(commandBuffer, m_DepthStencilView->GetInfo().Resource,
                                     ResourceState::DepthStencilWrite, m_Info.FinalDepthStencilState,
                                     ResourceStage::Graphics, ResourceStage::Graphics);
        }
    }
}

RenderPass::~RenderPass() {}

void RenderPass::DefaultRenderPassInfo(RenderPassInfo& info, uint32_t width, uint32_t height)
{
    info.ClearRenderTargets = LoadOp::DontCare;
    info.ClearDepth = LoadOp::DontCare;
    info.ClearStencil = LoadOp::DontCare;
    info.InitialRenderTargetState = ResourceState::RenderTarget;
    info.FinalRenderTargetState = ResourceState::RenderTarget;
    info.InitialDepthStencilState = ResourceState::DepthStencilWrite;
    info.FinalDepthStencilState = ResourceState::DepthStencilWrite;
    info.extent.width = width;
    info.extent.height = height;
}
