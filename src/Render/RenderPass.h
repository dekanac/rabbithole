#pragma once

#include <vector>

#include "Render/Vulkan/VulkanTypes.h"

class VulkanDevice;
class VulkanImageView;
class VulkanCommandBuffer;

struct RenderPassInfo
{
    LoadOp ClearRenderTargets = LoadOp::DontCare;
    LoadOp ClearDepth = LoadOp::DontCare;
    LoadOp ClearStencil = LoadOp::DontCare;
    ResourceState InitialRenderTargetState = ResourceState::None;
    ResourceState FinalRenderTargetState = ResourceState::None;
    ResourceState InitialDepthStencilState = ResourceState::None;
    ResourceState FinalDepthStencilState = ResourceState::None;
    Extent2D extent = {0, 0};
};

class RenderPass
{
  public:
    RenderPass(VulkanDevice& device, const std::vector<VulkanImageView*> renderTargetViews,
               const VulkanImageView* depthStencilView, const RenderPassInfo& info,
               const char* name);
    ~RenderPass();

    static void DefaultRenderPassInfo(RenderPassInfo& info, uint32_t width, uint32_t height);

    void BeginRenderPass(VulkanCommandBuffer& commandBuffer);
    void EndRenderPass(VulkanCommandBuffer& commandBuffer);

  private:
    VulkanDevice& m_Device;

    std::vector<VulkanImageView*> m_RenderTargetViews;
    const VulkanImageView* m_DepthStencilView;
    RenderPassInfo m_Info;

    VkExtent2D m_Extent;
};