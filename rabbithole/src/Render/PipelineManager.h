#pragma once

#include "common.h"

#include <unordered_map>

#include "Render/Vulkan/Include/VulkanWrapper.h"

struct ComputePipelineKey
{
	uint32_t m_ComputeShaderCRC;
	uint32_t m_ComputeShaderEntryPointCRC;

	bool operator < (const ComputePipelineKey& k) const;
	bool operator == (const ComputePipelineKey& k) const;
};

struct GraphicsPipelineKey
{
	uint32_t m_VertexShaderCRC;
	uint32_t m_VertexShaderEntryPointCRC;
	uint32_t m_PixelShaderCRC;
	uint32_t m_PixelShaderEntryPointCRC;
	uint32_t m_Topology;
	uint32_t m_PolygonMode;
	uint32_t m_CullMode;
	uint32_t m_Frontface;
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

typedef std::vector<uint32_t> FramebufferKey;
typedef std::vector<uint32_t> DescriptorSetKey;
typedef std::vector<uint32_t> DescriptorKey;

template<>
struct std::hash<GraphicsPipelineKey>
{
	inline uint32_t operator()(const GraphicsPipelineKey& x) const
	{
		return (uint32_t)crc32_fast((const void*)&x, sizeof(x));
	}
};

template<>
struct std::hash<ComputePipelineKey>
{
	inline uint32_t operator()(const ComputePipelineKey& x) const
	{
		return (uint32_t)crc32_fast((const void*)&x, sizeof(x));
	}
};

template<>
struct std::hash<RenderPassKey>
{
	inline uint32_t operator()(const RenderPassKey& x) const
	{
		return (uint32_t)crc32_fast((const void*)&x, sizeof(x));
	}
};

struct VectorHasher {

	std::size_t operator()(std::vector<uint32_t> const& vec) const
	{
		std::size_t seed = vec.size();

		for (auto& i : vec)
		{
			seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
		return seed;
	}
};

class Pipeline : public VulkanPipeline
{
public:
	Pipeline(VulkanDevice& device, PipelineConfigInfo& configInfo, PipelineType type)
		: VulkanPipeline(device, configInfo, type) {}
};

class GraphicsPipeline : public Pipeline
{
public:
	GraphicsPipeline(VulkanDevice& device, PipelineConfigInfo& configInfo)
		: Pipeline(device, configInfo, PipelineType::Graphics) {}

	virtual void Bind(VkCommandBuffer commandBuffer) override;
};

class ComputePipeline : public Pipeline
{
public:
	ComputePipeline(VulkanDevice& device, PipelineConfigInfo& configInfo)
		: Pipeline(device, configInfo, PipelineType::Compute) {}

	virtual void Bind(VkCommandBuffer commandBuffer) override;
};

class PipelineManager
{
	SingletonClass(PipelineManager);

public:
	std::unordered_map<GraphicsPipelineKey, GraphicsPipeline*>					m_GraphicPipelines;
	std::unordered_map<ComputePipelineKey, ComputePipeline*>					m_ComputePipelines;
	std::unordered_map<RenderPassKey, VulkanRenderPass*>						m_RenderPasses;
	std::unordered_map<FramebufferKey, VulkanFramebuffer*, VectorHasher>		m_Framebuffers;
	std::unordered_map<DescriptorSetKey, VulkanDescriptorSet*, VectorHasher>	m_DescriptorSets;
	std::unordered_map<DescriptorKey, VulkanDescriptor*, VectorHasher>			m_Descriptors;

	VulkanPipeline*			FindOrCreateGraphicsPipeline(VulkanDevice& device, PipelineConfigInfo& pipelineInfo);
	VulkanPipeline*			FindOrCreateComputePipeline(VulkanDevice& device, PipelineConfigInfo& pipelineInfo);
	VulkanRenderPass*		FindOrCreateRenderPass(VulkanDevice& device, const std::vector<VulkanImageView*>& renderTargets, const VulkanImageView* depthStencil, RenderPassConfigInfo& renderPassInfo);
	VulkanFramebuffer*		FindOrCreateFramebuffer(VulkanDevice& device, const std::vector<VulkanImageView*>& renderTargets, const VulkanImageView* depthStencil, const VulkanRenderPass* renderpass, const VulkanFramebufferInfo& framebufferInfo);
	VulkanDescriptorSet*	FindOrCreateDescriptorSet(VulkanDevice& device, const VulkanDescriptorPool* desciptorPool, const VulkanDescriptorSetLayout* descriptorSetLayout, const std::vector<VulkanDescriptor*>& descriptors);
};