#pragma once

#include "common.h"

#include <unordered_map>

#include "Render/Vulkan/Include/VulkanWrapper.h"

struct ComputePipelineKey
{
	ComputePipelineKey() { memset(this, 0, sizeof(ComputePipelineKey)); }

	uint32_t computeShaderCRC;
	uint32_t computeShaderEntryPointCRC;

	bool operator < (const ComputePipelineKey& k) const;
	bool operator == (const ComputePipelineKey& k) const;
};

struct GraphicsPipelineKey
{
	GraphicsPipelineKey() { memset(this, 0, sizeof(GraphicsPipelineKey)); }

	uint32_t vertexShaderCRC;
	uint32_t vertexShaderEntryPointCRC;
	uint32_t pixelShaderCRC;
	uint32_t pixelShaderEntryPointCRC;
	uint32_t topology;
	uint32_t polygonMode;
	uint32_t cullMode;
	uint32_t frontface;
	VkPipelineColorBlendAttachmentState blendAttachmentStates[MaxRenderTargetCount];

	bool operator < (const GraphicsPipelineKey& k) const;
	bool operator == (const GraphicsPipelineKey& k) const;
};

struct AttachmentDescription
{
	int32_t id = -1;
	uint8_t format : 8;
	uint8_t samples : 4;
	uint8_t initialLayout : 4;
	uint8_t finalLayout : 4;
	uint8_t loadOp : 2;
	uint8_t storeOp : 2;
};

struct RenderPassKey
{
	RenderPassKey() { memset(this, 0, sizeof(RenderPassKey)); }

	AttachmentDescription attachmentDescriptions[MaxRenderTargetCount];
	AttachmentDescription depthStencilAttachmentDescription;

	bool operator < (const RenderPassKey& k) const;
	bool operator == (const RenderPassKey& k) const;
};

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
	Pipeline(VulkanDevice& device, PipelineInfo& pipelineInfo, PipelineType type)
		: VulkanPipeline(device, pipelineInfo, type) {}
};

class GraphicsPipeline : public Pipeline
{
public:
	GraphicsPipeline(VulkanDevice& device, PipelineInfo& pipelineInfo)
		: Pipeline(device, pipelineInfo, PipelineType::Graphics) {}

	virtual void Bind(VulkanCommandBuffer& commandBuffer) override;
};

class ComputePipeline : public Pipeline
{
public:
	ComputePipeline(VulkanDevice& device, PipelineInfo& pipelineInfo)
		: Pipeline(device, pipelineInfo, PipelineType::Compute) {}

	virtual void Bind(VulkanCommandBuffer& commandBuffer) override;
};

class RayTracingPipeline : public Pipeline
{
public:
	RayTracingPipeline(VulkanDevice& device, PipelineInfo& pipelineInfo)
		: Pipeline(device, pipelineInfo, PipelineType::RayTracing) {}

	virtual void Bind(VulkanCommandBuffer& commandBuffer) override;
};

class PipelineManager
{
public:
	void Destroy();

private:
	std::unordered_map<GraphicsPipelineKey, GraphicsPipeline*>					m_GraphicPipelines;
	std::unordered_map<ComputePipelineKey, ComputePipeline*>					m_ComputePipelines;
	std::unordered_map<RenderPassKey, RenderPass*>								m_RenderPasses;
	std::unordered_map<DescriptorSetKey, VulkanDescriptorSet*, VectorHasher>	m_DescriptorSets;
	std::unordered_map<DescriptorKey, VulkanDescriptor*, VectorHasher>			m_Descriptors;

public:
	VulkanPipeline*			FindOrCreateGraphicsPipeline(VulkanDevice& device, PipelineInfo& pipelineInfo);
	VulkanPipeline*			FindOrCreateComputePipeline(VulkanDevice& device, PipelineInfo& pipelineInfo);
	RenderPass*				FindOrCreateRenderPass(VulkanDevice& device, const std::vector<VulkanImageView*>& renderTargets, const VulkanImageView* depthStencil, RenderPassInfo& renderPassInfo);
	VulkanDescriptorSet*	FindOrCreateDescriptorSet(VulkanDevice& device, const VulkanDescriptorPool* desciptorPool, const VulkanDescriptorSetLayout* descriptorSetLayout, const std::vector<VulkanDescriptor*>& descriptors);
	
	std::unordered_map<DescriptorKey, VulkanDescriptor*, VectorHasher>& GetDescriptors() { return m_Descriptors; }
};