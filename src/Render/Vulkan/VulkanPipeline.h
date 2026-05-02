#pragma once

#include <common.h>

#include "Logger/Logger.h"
#include "Render/Raytracing.h"
#include "VulkanTypes.h"
#include <vulkan/vulkan.h>

#include <array>
#include <string>
#include <vector>

#include <crc32/Crc32.h>

const uint8_t MaxRenderTargetCount = 6;
const uint8_t MaxRTShaders = 8;

class VulkanImageView;
class VulkanDevice;
class VulkanCommandBuffer;
class VulkanDescriptorSetLayout;
class VulkanDescriptorSet;
class VulkanDescriptorPool;
class VulkanDescriptor;
class Shader;

class VulkanPipelineLayout
{
  public:
    VulkanPipelineLayout(VulkanDevice& device,
                         std::vector<VulkanDescriptorSetLayout*>& descriptorSetLayouts,
                         const std::vector<VkPushConstantRange>& pushConsts);
    ~VulkanPipelineLayout();

    const VkPipelineLayout GetVkHandle() const { return m_PipelineLayout; }

  private:
    VulkanDevice& m_Device;

    VkPipelineLayout m_PipelineLayout;
};

class PipelineInfo
{
  public:
    PipelineInfo();
    // void SetVertexBinding(const VertexBinding* vertexBinding);
    void SetTopology(const Topology topology);
    void SetMultisampleType(const MultisampleType multisampleType);
    void SetDepthWriteEnabled(const bool enabled);
    void SetDepthTestEnabled(const bool enabled);
    void SetDepthCompare(const CompareOperation compareFunction);
    void SetDepthBias(const float depthBiasSlope, const float depthBias,
                      const float depthBiasClamp);
    void SetStencilEnable(const bool enabled);
    void SetStencilOperation(const StencilOperation stencilFail, const StencilOperation depthFail,
                             const StencilOperation stencilPass);
    void SetStencilFunction(const CompareOperation compareOperation, const uint8_t compareMask);
    void SetStencilWriteMask(const uint8_t mask);
    void SetWireFrameEnabled(const bool enabled);
    void SetCullMode(const CullMode mode);
    void SetWindingOrder(const WindingOrder winding);
    void SetAttachmentCount(const uint32_t attachmentCount);
    void SetAlphaBlendEnabled(const uint32_t mrtIndex, const bool enabled);
    void SetAlphaBlendFunction(const uint32_t mrtIndex, const BlendValue srcBlend,
                               const BlendValue destBlend);
    void SetAlphaBlendFunction(const uint32_t mrtIndex, const BlendValue srcColorBlend,
                               const BlendValue dstColorBlend, const BlendValue srcAlphaBlend,
                               const BlendValue dstAlphablend);
    void SetAlphaBlendOperation(const uint32_t mrtIndex, const BlendOperation colorOperation);
    void SetAlphaBlendOperation(const uint32_t mrtIndex, const BlendOperation colorOperation,
                                const BlendOperation alphaOperation);
    void SetColorWriteMask(const uint32_t mrtIndex, const ColorWriteMaskFlags mask);
    void SetColorWriteMask(const uint32_t mrtIndex, const uint32_t mrtCount,
                           const ColorWriteMaskFlags masks[]);

    void SetFormats(const std::vector<VulkanImageView*>& colorAttachments,
                    const VulkanImageView* depthStencil);

    Shader* vertexShader = nullptr;
    std::string vsEntryPoint = "main";
    Shader* pixelShader = nullptr;
    std::string psEntryPoint = "main";
    Shader* computeShader = nullptr;
    std::string csEntryPoint = "main";
    std::array<Shader*, MaxRTShaders> rayTracingShaders = {nullptr};

    VkViewport viewport{};
    VkRect2D scissor{};
    VkPipelineViewportStateCreateInfo viewportInfo{};
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
    VkPipelineMultisampleStateCreateInfo multisampleInfo{};
    VkPipelineColorBlendAttachmentState colorBlendAttachment[MaxRenderTargetCount]{};
    VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
    VkPipelineLayout pipelineLayout = nullptr;
    uint32_t colorAttachmentCount = 0;
    std::array<VkFormat, MaxRenderTargetCount> colorAttachmentFormats = {VK_FORMAT_UNDEFINED};
    VkFormat depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    VkFormat stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
};

class VulkanPipeline
{
  public:
    VulkanPipeline(VulkanDevice& device, PipelineInfo& pipelineInfo,
                   PipelineType type = PipelineType::Graphics);
    ~VulkanPipeline();

    NonCopyableAndMovable(VulkanPipeline);

    virtual void Bind(VulkanCommandBuffer& commandBuffer)
    {
        ASSERT(false, "Should not be called!");
    }

    static void DefaultPipelineInfo(PipelineInfo*& pipelineInfo, uint32_t width, uint32_t height);
    const VulkanDescriptorSetLayout* GetDescriptorSetLayout() { return m_DescriptorSetLayout; }
    const VulkanPipelineLayout* GetPipelineLayout() const { return m_PipelineLayout; }
    const PipelineType GetType() const { return m_Type; }
#if defined(VULKAN_HWRT)
    const RayTracing::ShaderBindingTables& GetBindingTables() const { return m_BindingTables; }
#endif

    VkPipeline GetVkHandle() { return m_Pipeline; }

  private:
    void CreateGraphicsPipeline();
    void CreateComputePipeline();
    void CreateRayTracingPipeline();

    VulkanDevice& m_VulkanDevice;
    PipelineInfo& m_PipelineInfo;
    VulkanPipelineLayout* m_PipelineLayout;
    VulkanDescriptorSetLayout* m_DescriptorSetLayout;
    PipelineType m_Type;

#if defined(VULKAN_HWRT)
    RayTracing::ShaderBindingTables m_BindingTables{};
#endif

  protected:
    VkPipeline m_Pipeline;
};
