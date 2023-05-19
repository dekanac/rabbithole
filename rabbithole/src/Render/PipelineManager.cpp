#include "PipelineManager.h"

#include "Logger/Logger.h"
#include "Render/Vulkan/VulkanPipeline.h"
#include "Render/Converters.h"
#include "Render/RenderPass.h"
#include "Render/Shader.h"

bool GraphicsPipelineKey::operator<(const GraphicsPipelineKey& k) const
{
	return memcmp(this, &k, sizeof(GraphicsPipelineKey)) < 0;
}

bool GraphicsPipelineKey::operator==(const GraphicsPipelineKey& k) const
{
	return memcmp(this, &k, sizeof(GraphicsPipelineKey)) == 0;
}

bool ComputePipelineKey::operator<(const ComputePipelineKey& k) const
{
	return memcmp(this, &k, sizeof(ComputePipelineKey)) < 0;
}

bool ComputePipelineKey::operator==(const ComputePipelineKey& k) const
{
	return memcmp(this, &k, sizeof(ComputePipelineKey)) == 0;
}

bool RayTracingPipelineKey::operator<(const RayTracingPipelineKey& k) const
{
	return memcmp(this, &k, sizeof(RayTracingPipelineKey)) < 0;
}

bool RayTracingPipelineKey::operator==(const RayTracingPipelineKey& k) const
{
	return memcmp(this, &k, sizeof(RayTracingPipelineKey)) == 0;
}

bool RenderPassKey::operator<(const RenderPassKey& k) const
{
	return memcmp(this, &k, sizeof(RenderPassKey)) < 0;
}

bool RenderPassKey::operator==(const RenderPassKey& k) const
{
	return memcmp(this, &k, sizeof(RenderPassKey)) == 0;
}

void PipelineManager::Destroy()
{
	for (auto object : m_GraphicPipelines) { delete object.second; }
	for (auto object : m_ComputePipelines) { delete object.second; }
	for (auto object : m_RenderPasses) { delete object.second; }
}

VulkanPipeline* PipelineManager::FindOrCreateGraphicsPipeline(VulkanDevice& device, PipelineInfo& pipelineInfo)
{
	GraphicsPipelineKey key{};

	key.vertexShaderCRC = pipelineInfo.vertexShader ? pipelineInfo.vertexShader->GetHash() : 0;
	key.vertexShaderEntryPointCRC = (uint32_t)crc32_fast((const void*)pipelineInfo.vsEntryPoint.c_str(), pipelineInfo.vsEntryPoint.length());
	key.pixelShaderCRC = pipelineInfo.pixelShader ? pipelineInfo.pixelShader->GetHash() : 0;
	key.pixelShaderEntryPointCRC = (uint32_t)crc32_fast((const void*)pipelineInfo.psEntryPoint.c_str(), pipelineInfo.psEntryPoint.length());
	key.topology = pipelineInfo.inputAssemblyInfo.topology;

	key.polygonMode = pipelineInfo.rasterizationInfo.polygonMode;
	key.cullMode = pipelineInfo.rasterizationInfo.cullMode;
	key.frontface = pipelineInfo.rasterizationInfo.frontFace;
	memcpy(&key.blendAttachmentStates, &pipelineInfo.colorBlendAttachment, sizeof(VkPipelineColorBlendAttachmentState) * MaxRenderTargetCount);

	//fill the key, find the pipeline in the map
	//if it is not in the map, create and add to map

	auto pipelineIterator = m_GraphicPipelines.find(key);
	if (pipelineIterator != m_GraphicPipelines.end())
	{
		return pipelineIterator->second;
	}
	else
	{
		LOG_WARNING("If you're seeing this every frame, you're doing something wrong! Check GraphicsPipelineKey!");
		GraphicsPipeline* pipeline = new GraphicsPipeline(device, pipelineInfo);
		m_GraphicPipelines[key] = pipeline;
		return pipeline;
	}
}

VulkanPipeline* PipelineManager::FindOrCreateComputePipeline(VulkanDevice& device, PipelineInfo& pipelineInfo)
{
	//for now the key is only CRC of compute shader and CRC of string of entry point
	ComputePipelineKey key{};

	key.computeShaderCRC = pipelineInfo.computeShader->GetHash();
	key.computeShaderEntryPointCRC = (uint32_t)crc32_fast((const void*)pipelineInfo.csEntryPoint.c_str(), pipelineInfo.csEntryPoint.length());

	auto pipeline = m_ComputePipelines.find(key);

	if (pipeline != m_ComputePipelines.end())
	{
		return pipeline->second;
	}
	else
	{
		LOG_WARNING("If you're seeing this every frame, you're doing something wrong! Check ComputePipelineKey!");
		ComputePipeline* newPipeline = new ComputePipeline(device, pipelineInfo);
		m_ComputePipelines[key] = newPipeline;
		return newPipeline;
	}
}

VulkanPipeline* PipelineManager::FindOrCreateRayTracingPipeline(VulkanDevice& device, PipelineInfo& pipelineInfo)
{
	//for now the key is only CRCs of ray tracing shaders
	RayTracingPipelineKey key{};
	auto& rtShaders = pipelineInfo.rayTracingShaders;

	for (uint32_t i = 0; i < MaxRTShaders; i++)
	{
		key.rayTracingShaderCRCs[i] = rtShaders[i] != nullptr ? rtShaders[i]->GetHash() : 0;
	}

	auto pipeline = m_RayTracingPipelines.find(key);

	if (pipeline != m_RayTracingPipelines.end())
	{
		return pipeline->second;
	}
	else
	{
		LOG_WARNING("If you're seeing this every frame, you're doing something wrong! Check RayTracingPipelineKey!");
		RayTracingPipeline* newPipeline = new RayTracingPipeline(device, pipelineInfo);
		m_RayTracingPipelines[key] = newPipeline;
		return newPipeline;
	}
}

RenderPass* PipelineManager::FindOrCreateRenderPass(VulkanDevice& device, const std::vector<VulkanImageView*>& renderTargets, const VulkanImageView* depthStencil, RenderPassInfo& renderPassInfo)
{
	RenderPassKey key{};

	for (size_t i = 0; i < renderTargets.size(); i++)
	{
		key.attachmentDescriptions[i].id = renderTargets[i]->GetID();
		key.attachmentDescriptions[i].format = GetVkFormatFrom(renderTargets[i]->GetFormat());
		key.attachmentDescriptions[i].samples = GetVkSampleFlagsFrom(renderTargets[i]->GetInfo().Resource->GetInfo().MultisampleType);
		key.attachmentDescriptions[i].loadOp = GetVkLoadOpFrom(renderPassInfo.ClearRenderTargets);
		key.attachmentDescriptions[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		key.attachmentDescriptions[i].initialLayout = GetVkImageLayoutFrom(renderPassInfo.InitialRenderTargetState);
		key.attachmentDescriptions[i].finalLayout = GetVkImageLayoutFrom(renderPassInfo.FinalRenderTargetState);
	}

	if (depthStencil != nullptr)
	{
		key.depthStencilAttachmentDescription.id = depthStencil->GetID();
		key.depthStencilAttachmentDescription.format = GetVkFormatFrom(depthStencil->GetFormat());
		key.depthStencilAttachmentDescription.samples = GetVkSampleFlagsFrom(depthStencil->GetInfo().Resource->GetInfo().MultisampleType);
		key.depthStencilAttachmentDescription.loadOp = GetVkLoadOpFrom(renderPassInfo.ClearDepth);
		key.depthStencilAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		key.depthStencilAttachmentDescription.initialLayout = GetVkImageLayoutFrom(renderPassInfo.InitialDepthStencilState);
		key.depthStencilAttachmentDescription.finalLayout = GetVkImageLayoutFrom(renderPassInfo.FinalDepthStencilState);
	}

	auto renderpass = m_RenderPasses.find(key);
	if (renderpass != m_RenderPasses.end())
	{
		return renderpass->second;
	}
	else
	{
		LOG_WARNING("If you're seeing this every frame, you're doing something wrong! Check RenderPassKey!");
		RenderPass* newRenderPass = new RenderPass(device, renderTargets, depthStencil, renderPassInfo, "RenderPass");
		m_RenderPasses[key] = newRenderPass;
		return newRenderPass;
	}

}

VulkanDescriptorSet* PipelineManager::FindOrCreateDescriptorSet(VulkanDevice& device, const VulkanDescriptorPool* desciptorPool, const VulkanDescriptorSetLayout* descriptorSetLayout, const std::vector<VulkanDescriptor*>& descriptors)
{
	DescriptorSetKey key;

	for (size_t i = 0; i < descriptors.size(); i++)
	{
		switch (descriptors[i]->GetDescriptorInfo().Type)
		{
		case DescriptorType::UniformBuffer:
			key.push_back(descriptors[i]->GetDescriptorInfo().buffer->GetID());
			key.push_back((uint32_t)descriptors[i]->GetDescriptorInfo().Type);
			break;
		case DescriptorType::CombinedSampler:
			key.push_back(descriptors[i]->GetDescriptorInfo().imageView->GetID());
			key.push_back((uint32_t)descriptors[i]->GetDescriptorInfo().Type);
			break;
		case DescriptorType::SampledImage:
			key.push_back(descriptors[i]->GetDescriptorInfo().imageView->GetID());
			key.push_back((uint32_t)descriptors[i]->GetDescriptorInfo().Type);
			break;
		case DescriptorType::Sampler:
			key.push_back(descriptors[i]->GetDescriptorInfo().imageSampler->GetID());
			key.push_back((uint32_t)descriptors[i]->GetDescriptorInfo().Type);
			break;
		case DescriptorType::StorageImage:
			key.push_back(descriptors[i]->GetDescriptorInfo().imageView->GetID());
			key.push_back((uint32_t)descriptors[i]->GetDescriptorInfo().Type);
			break;
		case DescriptorType::StorageBuffer:
			key.push_back(descriptors[i]->GetDescriptorInfo().buffer->GetID());
			key.push_back((uint32_t)descriptors[i]->GetDescriptorInfo().Type);
			break;
#if defined(VULKAN_HWRT)
		case DescriptorType::AccelerationStructure:
			key.push_back(descriptors[i]->GetDescriptorInfo().accelerationStructure->GetID());
			key.push_back((uint32_t)descriptors[i]->GetDescriptorInfo().Type);
			break;
#endif
		}
	}

	auto descriptorset = m_DescriptorSets.find(key);
	if (descriptorset != m_DescriptorSets.end())
	{
		return descriptorset->second;
	}
	else
	{
		LOG_WARNING("If you're seeing this every frame, you're doing something wrong! Check DescriptorSetKey!");
		VulkanDescriptorSet* newDescSet = new VulkanDescriptorSet(&device, desciptorPool, descriptorSetLayout, descriptors, "descriptorset");
		m_DescriptorSets[key] = newDescSet;
		return newDescSet;
	}

	return nullptr;
}

void GraphicsPipeline::Bind(VulkanCommandBuffer& commandBuffer)
{
	vkCmdBindPipeline(GET_VK_HANDLE(commandBuffer), VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
}

void ComputePipeline::Bind(VulkanCommandBuffer& commandBuffer)
{
	vkCmdBindPipeline(GET_VK_HANDLE(commandBuffer), VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
}

void RayTracingPipeline::Bind(VulkanCommandBuffer& commandBuffer)
{
	vkCmdBindPipeline(GET_VK_HANDLE(commandBuffer), VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_Pipeline);
}
