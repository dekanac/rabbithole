#pragma once

#include "VulkanDevice.h"
#include "VulkanPipeline.h"
#include "VulkanCommandBuffer.h"

#include "Logger/Logger.h"

#include <string>
#include <vector>

#include <crc32/Crc32.h>

const uint8_t MaxRenderTargetCount = 5;

class VulkanDescriptorSetLayout;
class VulkanDescriptorSet;
class VulkanDescriptorPool;
class VulkanDescriptor;
class Shader;
struct RenderPassConfigInfo;

class PipelineConfigInfo 
{
public:
	PipelineConfigInfo();
	//void SetVertexBinding(const VertexBinding* vertexBinding);
	void SetTopology(const Topology topology);
	void SetMultisampleType(const MultisampleType multisampleType);
	void SetDepthWriteEnabled(const bool enabled);
	void SetDepthTestEnabled(const bool enabled);
	void SetDepthCompare(const CompareOperation compareFunction);
	void SetDepthBias(const float depthBiasSlope, const float depthBias, const float depthBiasClamp);
	void SetStencilEnable(const bool enabled);
	void SetStencilOperation(const StencilOperation stencilFail, const StencilOperation depthFail, const StencilOperation stencilPass);
	void SetStencilFunction(const CompareOperation compareOperation, const uint8_t compareMask);
	void SetStencilWriteMask(const uint8_t mask);
	void SetWireFrameEnabled(const bool enabled);
	void SetCullMode(const CullMode mode);
	void SetWindingOrder(const WindingOrder winding);
	void SetAttachmentCount(const uint32_t attachmentCount);
	void SetAlphaBlendEnabled(const uint32_t mrtIndex, const bool enabled);
	void SetAlphaBlendFunction(const uint32_t mrtIndex, const BlendValue srcBlend, const BlendValue destBlend);
	void SetAlphaBlendFunction(const uint32_t mrtIndex, const BlendValue srcColorBlend, const BlendValue dstColorBlend, const BlendValue srcAlphaBlend, const BlendValue dstAlphablend);
	void SetAlphaBlendOperation(const uint32_t mrtIndex, const BlendOperation colorOperation);
	void SetAlphaBlendOperation(const uint32_t mrtIndex, const BlendOperation colorOperation, const BlendOperation alphaOperation);
	void SetColorWriteMask(const uint32_t mrtIndex, const ColorWriteMaskFlags mask);
	void SetColorWriteMask(const uint32_t mrtIndex, const uint32_t mrtCount, const ColorWriteMaskFlags masks[]);

	NonCopyableAndMovable(PipelineConfigInfo);

	Shader*									vertexShader;
	std::string vsEntryPoint = "main";
	Shader*									pixelShader;
	std::string psEntryPoint = "main";
	Shader*									computeShader;
	std::string csEntryPoint = "main";

	VkViewport								viewport;
	VkRect2D								scissor;
	VkPipelineViewportStateCreateInfo		viewportInfo;
	VkPipelineInputAssemblyStateCreateInfo	inputAssemblyInfo;
	VkPipelineRasterizationStateCreateInfo	rasterizationInfo;
	VkPipelineMultisampleStateCreateInfo	multisampleInfo;
	VkPipelineColorBlendAttachmentState		colorBlendAttachment[MaxRenderTargetCount];
	VkPipelineColorBlendStateCreateInfo		colorBlendInfo;
	VkPipelineDepthStencilStateCreateInfo	depthStencilInfo;
	VkPipelineLayout						pipelineLayout = nullptr;
	VulkanRenderPass*						renderPass = nullptr;
	uint32_t								subpass = 0;

};

class VulkanPipeline 
{
public:
	VulkanPipeline(VulkanDevice& device, PipelineConfigInfo& configInfo, PipelineType type = PipelineType::Graphics);
	~VulkanPipeline();

	NonCopyableAndMovable(VulkanPipeline);

	virtual void					 Bind(VulkanCommandBuffer& commandBuffer) { ASSERT(false, "Should not be called!"); }

	static void						 DefaultPipelineConfigInfo(PipelineConfigInfo*& configInfo, uint32_t width, uint32_t height);
	const VulkanDescriptorSetLayout* GetDescriptorSetLayout() { return m_DescriptorSetLayout; }
	const VkPipelineLayout*			 GetPipelineLayout() const { return &m_PipelineLayout; }
	const PipelineType				 GetType() const { return m_Type; }

	VkPipeline						 GetVkHandle() { return m_Pipeline; }

private:
	void							 CreateGraphicsPipeline();
	void							 CreateComputePipeline();

	VulkanDevice&					 m_VulkanDevice;
	PipelineConfigInfo&				 m_PipelineInfo;
	VkPipelineLayout				 m_PipelineLayout;
	VulkanDescriptorSetLayout*		 m_DescriptorSetLayout;
	VulkanRenderPass*				 m_RenderPass;
	PipelineType					 m_Type;
									 
protected:							 
	VkPipeline						 m_Pipeline;
};
