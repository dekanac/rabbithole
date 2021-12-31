#pragma once

#include "VulkanDevice.h"
#include "VulkanPipeline.h"

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

struct DescriptorSetLayoutBinding
{
	uint32_t descriptorCount : 32;
	uint16_t binding : 16;
	uint8_t descriptorType : 4;
	uint8_t stageFlags : 6;
};

struct GraphicsPipelineKey
{
	//TODO: need to add viewport here somehow
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

struct AttachmentDescription
{
	uint8_t format : 8;
	uint8_t samples : 4;
	uint8_t initialLayout : 4;
	uint8_t finalLayout : 4;
	uint8_t loadOp : 2;
	uint8_t storeOp : 2;
};

struct RenderPassKey
{
	AttachmentDescription attachmentDescriptions[MaxRenderTargetCount];
	AttachmentDescription depthStencilAttachmentDescription;

	bool operator < (const RenderPassKey& k) const;
	bool operator == (const RenderPassKey& k) const;
};

template<>
struct std::hash<RenderPassKey>
{
	inline uint32_t operator()(const RenderPassKey& x) const
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
	VkPipeline						 GetVkPipeline() { return m_GraphicsPipeline; }
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

typedef std::vector<uint32_t> FramebufferKey;
typedef std::vector<uint32_t> DescriptorSetKey;


struct VectorHasher {

	std::size_t operator()(std::vector<uint32_t> const& vec) const {
		std::size_t seed = vec.size();
		for (auto& i : vec) {
			seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
		return seed;
	}
};

//for now its the same thing, vector of attachments

class PipelineManager
{
	SingletonClass(PipelineManager)

public:
	std::unordered_map<GraphicsPipelineKey, VulkanPipeline*> m_GraphicPipelines;
	std::unordered_map<RenderPassKey, VulkanRenderPass*>	 m_RenderPasses;
	std::unordered_map<FramebufferKey, VulkanFramebuffer*, VectorHasher>   m_Framebuffers;
	std::unordered_map<DescriptorSetKey, VulkanDescriptorSet*, VectorHasher> m_DescriptorSets;

	VulkanPipeline* FindOrCreateGraphicsPipeline(VulkanDevice& device, PipelineConfigInfo& pipelineInfo);
	VulkanRenderPass* FindOrCreateRenderPass(VulkanDevice& device, const std::vector<VulkanImageView*> renderTargets, const VulkanImageView* depthStencil, RenderPassConfigInfo& renderPassInfo);
	VulkanFramebuffer* FindOrCreateFramebuffer(VulkanDevice& device, const std::vector<VulkanImageView*> renderTargets, const VulkanImageView* depthStencil, const VulkanRenderPass* renderpass, uint32_t width, uint32_t height);
	VulkanDescriptorSet* FindOrCreateDescriptorSet(VulkanDevice& device, const VulkanDescriptorPool* desciptorPool, const VulkanDescriptorSetLayout* descriptorSetLayout, const std::vector<VulkanDescriptor*> descriptors);
};