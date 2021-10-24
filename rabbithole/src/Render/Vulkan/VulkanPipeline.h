#pragma once

#include "VulkanDevice.h"
#include "VulkanPipeline.h"

#include <string>
#include <vector>

#include <crc32/Crc32.h>

const uint8_t MaxRenderTargetCount = 4;

class VulkanDescriptorSetLayout;
class VulkanDescriptorSet;
class Shader;

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

	PipelineConfigInfo(const PipelineConfigInfo&) = delete;
	PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

	Shader*									vertexShader;
	Shader*									pixelShader;

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

struct GraphicsPipelineKey
{
	uint32_t m_VertexShaderCRC;
	uint32_t m_PixelShaderCRC;
	uint32_t m_Topology;
	uint32_t m_Padding1;

	uint32_t m_PolygonMode;
	uint32_t m_CullMode;
	uint32_t m_Frontface;
	uint32_t m_Padding2;
	//TODO: implement this fully
	/*
	VkFormat m_RenderTargetFormats[MaxRenderTargetCount];
	VkSampleCountFlagBits m_RenderTargetSampleCount[MaxRenderTargetCount];
	VkFormat m_DepthStencilFormat;
	VkSampleCountFlagBits m_DepthStencilSampleCount;

	uint32_t m_DepthStencilIndex;
	uint32_t m_BlendStateIndex;
	uint32_t m_RasterizerStateIndex;
	uint32_t m_Padding2;
	*/

	bool operator < (const GraphicsPipelineKey& k) const;
	bool operator == (const GraphicsPipelineKey& k) const;
};

template<>
struct std::hash<GraphicsPipelineKey>
{
	inline uint32_t operator()(const GraphicsPipelineKey& x) const
	{
		return (uint32_t)crc32_fast((const void*)&x, sizeof(x));
	}
};

class VulkanPipeline 
{
public:
	VulkanPipeline(VulkanDevice& device, PipelineConfigInfo& configInfo);
	~VulkanPipeline();

	VulkanPipeline(const VulkanPipeline&) = delete;
	VulkanPipeline operator=(const VulkanPipeline&) = delete;

	void							 Bind(VkCommandBuffer commandBuffer);

	static void						 DefaultPipelineConfigInfo(PipelineConfigInfo*& configInfo, uint32_t width, uint32_t height);
	const VulkanDescriptorSetLayout* GetDescriptorSetLayout() { return m_DescriptorSetLayout; }
	const VkPipelineLayout*			 GetPipelineLayout() const { return &m_PipelineLayout; }
	void							 CreatePipeline();

private:

	VulkanDevice&							m_VulkanDevice;
	PipelineConfigInfo&						m_PipelineInfo;
	VkPipeline								m_GraphicsPipeline;
	VkPipelineLayout						m_PipelineLayout;
	VulkanDescriptorSetLayout*				m_DescriptorSetLayout;
	VulkanRenderPass*						m_RenderPass;
};

struct GraphicPipelineDescription
{

};

class PipelineManager 
{
	SingletonClass(PipelineManager)

public:
	std::unordered_map<GraphicsPipelineKey, VulkanPipeline*> m_GraphicPipelines;

	VulkanPipeline* FindOrCreateGraphicsPipeline(VulkanDevice& device, PipelineConfigInfo& pipelineInfo);
};
