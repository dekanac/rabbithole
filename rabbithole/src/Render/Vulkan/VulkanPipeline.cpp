#include "precomp.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "Shader.h"
#include "../Renderer.h"

VulkanPipeline::VulkanPipeline(VulkanDevice& device, PipelineConfigInfo& configInfo)
	: m_VulkanDevice{ device } 
	, m_PipelineInfo(configInfo)
	, m_RenderPass(configInfo.renderPass)
{
	CreatePipeline();
}

VulkanPipeline::~VulkanPipeline() 
{
	vkDestroyPipeline(m_VulkanDevice.GetGraphicDevice(), m_GraphicsPipeline, nullptr);
}


void VulkanPipeline::Bind(VkCommandBuffer commandBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
}

void VulkanPipeline::DefaultPipelineConfigInfo(PipelineConfigInfo*& configInfo, uint32_t width, uint32_t height) 
{

	configInfo->inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	configInfo->SetTopology(Topology::TriangleList);

	configInfo->viewport.x = 0.0f;
	configInfo->viewport.y = 0.0f;
	configInfo->viewport.width = static_cast<float>(width);
	configInfo->viewport.height = static_cast<float>(height);
	configInfo->viewport.minDepth = 0.0f;
	configInfo->viewport.maxDepth = 1.0f;

	configInfo->scissor.offset = { 0, 0 };
	configInfo->scissor.extent = { width, height };

	configInfo->viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	configInfo->viewportInfo.viewportCount = 1;
	configInfo->viewportInfo.pViewports = &configInfo->viewport;
	configInfo->viewportInfo.scissorCount = 1;
	configInfo->viewportInfo.pScissors = &configInfo->scissor;

	configInfo->rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	configInfo->rasterizationInfo.depthClampEnable = VK_FALSE;
	configInfo->rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	configInfo->rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
	configInfo->rasterizationInfo.lineWidth = 1.0f;
	configInfo->SetCullMode(CullMode::Back);
	configInfo->SetWindingOrder(WindingOrder::Clockwise);
	configInfo->SetDepthBias(0.0f, 0.0f, 0.0f);

	configInfo->multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	configInfo->multisampleInfo.sampleShadingEnable = VK_FALSE;
	configInfo->multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	configInfo->multisampleInfo.minSampleShading = 1.0f;           // Optional
	configInfo->multisampleInfo.pSampleMask = nullptr;             // Optional
	configInfo->multisampleInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
	configInfo->multisampleInfo.alphaToOneEnable = VK_FALSE;       // Optional

	configInfo->colorBlendAttachment[0].colorWriteMask = 0xf;
	configInfo->colorBlendAttachment[0].blendEnable = VK_FALSE;

	configInfo->colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	configInfo->colorBlendInfo.logicOpEnable = VK_FALSE;
	configInfo->colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
	configInfo->colorBlendInfo.attachmentCount = 1;
	configInfo->colorBlendInfo.pAttachments = configInfo->colorBlendAttachment;
	configInfo->colorBlendInfo.blendConstants[0] = 0.0f;  // Optional
	configInfo->colorBlendInfo.blendConstants[1] = 0.0f;  // Optional
	configInfo->colorBlendInfo.blendConstants[2] = 0.0f;  // Optional
	configInfo->colorBlendInfo.blendConstants[3] = 0.0f;  // Optional

	configInfo->depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	configInfo->depthStencilInfo.depthTestEnable = VK_TRUE;
	configInfo->depthStencilInfo.depthWriteEnable = VK_TRUE;
	configInfo->depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	configInfo->depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	configInfo->depthStencilInfo.minDepthBounds = 0.0f;  // Optional
	configInfo->depthStencilInfo.maxDepthBounds = 1.0f;  // Optional
	configInfo->depthStencilInfo.stencilTestEnable = VK_FALSE;
	configInfo->depthStencilInfo.front = {};  // Optional
	configInfo->depthStencilInfo.back = {};   // Optional
}

void VulkanPipeline::CreatePipeline()
{
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};

	VkPipelineShaderStageCreateInfo vertexShaderStage{};

	vertexShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderStage.stage = GetVkShaderStageFrom(m_PipelineInfo.vertexShader->GetInfo().Type);
	vertexShaderStage.module = m_PipelineInfo.vertexShader->GetModule();
	vertexShaderStage.pName = "main";
	vertexShaderStage.flags = 0;
	vertexShaderStage.pNext = nullptr;

	shaderStages.push_back(vertexShaderStage);

	VkPipelineShaderStageCreateInfo pixelShaderStage{};

	pixelShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	pixelShaderStage.stage = GetVkShaderStageFrom(m_PipelineInfo.pixelShader->GetInfo().Type);
	pixelShaderStage.module = m_PipelineInfo.pixelShader->GetModule();
	pixelShaderStage.pName = "main";
	pixelShaderStage.flags = 0;
	pixelShaderStage.pNext = nullptr;

	shaderStages.push_back(pixelShaderStage);

	m_DescriptorSetLayout = new VulkanDescriptorSetLayout(&m_VulkanDevice, { m_PipelineInfo.pixelShader, m_PipelineInfo.vertexShader }, "Main");
	m_PipelineLayout = *(m_DescriptorSetLayout->GetPipelineLayout());

	auto bindingDescriptions = Vertex::GetBindingDescriptions();		//TODO: abstract this
	auto attributeDescriptions = Vertex::GetAttributeDescriptions();	//TODO: abstract this
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = shaderStages.size();
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &m_PipelineInfo.inputAssemblyInfo;
	pipelineInfo.pViewportState = &m_PipelineInfo.viewportInfo;
	pipelineInfo.pRasterizationState = &m_PipelineInfo.rasterizationInfo;
	pipelineInfo.pMultisampleState = &m_PipelineInfo.multisampleInfo;
	pipelineInfo.pColorBlendState = &m_PipelineInfo.colorBlendInfo;
	pipelineInfo.pDepthStencilState = &m_PipelineInfo.depthStencilInfo;

#ifdef DYNAMIC_SCISSOR_AND_VIEWPORT_STATES
	VkDynamicState dynamicStates[2];
	VkDynamicState viewportState = VK_DYNAMIC_STATE_VIEWPORT;
	VkDynamicState scissorState = VK_DYNAMIC_STATE_SCISSOR;
	dynamicStates[0] = viewportState;
	dynamicStates[1] = scissorState;

	VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
	dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateInfo.dynamicStateCount = 2;
	dynamicStateInfo.pDynamicStates = &dynamicStates[0];

	pipelineInfo.pDynamicState = &dynamicStateInfo;
#else
	pipelineInfo.pDynamicState = nullptr;
#endif

	pipelineInfo.layout = *(m_DescriptorSetLayout->GetPipelineLayout());
	pipelineInfo.renderPass = m_RenderPass->GetVkRenderPass();
	pipelineInfo.subpass = m_PipelineInfo.subpass;

	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VULKAN_API_CALL(vkCreateGraphicsPipelines(m_VulkanDevice.GetGraphicDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline));
}

// void PipelineConfigInfo::SetVertexBinding(const VertexBinding* vertexBinding)
// {
// 	m_VertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
// 	m_VertexInputStateCreateInfo.pVertexBindingDescriptions = &get_st(vertexBinding)->m_VertexInputBindingDescription;
// 	m_VertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(get_st(vertexBinding)->m_VertexAttributeDescriptions.size());
// 	m_VertexInputStateCreateInfo.pVertexAttributeDescriptions = get_st(vertexBinding)->m_VertexAttributeDescriptions.data();
// }

void PipelineConfigInfo::SetTopology(const Topology topology)
{
	inputAssemblyInfo.topology = GetVkPrimitiveTopologyFrom(topology);
}

void PipelineConfigInfo::SetMultisampleType(const MultisampleType multiSampleType)
{
	multisampleInfo.rasterizationSamples = GetVkSampleFlagsFrom(multiSampleType);
	multisampleInfo.sampleShadingEnable = multiSampleType == MultisampleType::Sample_1 ? VK_FALSE : VK_TRUE;
}

void PipelineConfigInfo::SetDepthWriteEnabled(const bool enabled)
{
	depthStencilInfo.depthWriteEnable = enabled;
}

void PipelineConfigInfo::SetDepthTestEnabled(const bool enabled)
{
	depthStencilInfo.depthTestEnable = enabled;
}

void PipelineConfigInfo::SetDepthCompare(const CompareOperation compareFunction)
{
	depthStencilInfo.depthCompareOp = GetVkCompareOperationFrom(compareFunction);
}

void PipelineConfigInfo::SetDepthBias(const float depthBiasSlope, const float depthBias, const float depthBiasClamp)
{
	rasterizationInfo.depthBiasEnable = (depthBiasSlope != 0.0f) || (depthBias != 0.0f);
	rasterizationInfo.depthBiasSlopeFactor = depthBiasSlope;
	rasterizationInfo.depthBiasConstantFactor = depthBias;
	rasterizationInfo.depthBiasClamp = depthBiasClamp;
}

void PipelineConfigInfo::SetStencilEnable(const bool enabled)
{
	depthStencilInfo.stencilTestEnable = enabled;
}

void PipelineConfigInfo::SetStencilOperation(const StencilOperation stencilFail, const StencilOperation depthFail, const StencilOperation stencilPass)
{
	VkStencilOp stencilFailOp = GetVkStencilOpFrom(stencilFail);
	VkStencilOp depthFailOp = GetVkStencilOpFrom(depthFail);
	VkStencilOp stencilPassOp = GetVkStencilOpFrom(stencilPass);

	depthStencilInfo.front.failOp = stencilFailOp;
	depthStencilInfo.front.depthFailOp = depthFailOp;
	depthStencilInfo.front.passOp = stencilPassOp;

	depthStencilInfo.back.failOp = stencilFailOp;
	depthStencilInfo.back.depthFailOp = depthFailOp;
	depthStencilInfo.back.passOp = stencilPassOp;
}

void PipelineConfigInfo::SetStencilFunction(const CompareOperation compareOperation, const uint8_t compareMask)
{
	VkCompareOp stencilCompareOp = GetVkCompareOperationFrom(compareOperation);

	depthStencilInfo.front.compareOp = stencilCompareOp;
	depthStencilInfo.back.compareOp = stencilCompareOp;

	depthStencilInfo.front.compareMask = compareMask;
	depthStencilInfo.back.compareMask = compareMask;
}

void PipelineConfigInfo::SetStencilWriteMask(const uint8_t mask)
{
	depthStencilInfo.front.writeMask = mask;
	depthStencilInfo.back.writeMask = mask;
}

void PipelineConfigInfo::SetWireFrameEnabled(const bool enable)
{
	rasterizationInfo.polygonMode = enable ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
}

void PipelineConfigInfo::SetCullMode(const CullMode mode)
{
	rasterizationInfo.cullMode = mode == CullMode::None ? VK_CULL_MODE_NONE :
		mode == CullMode::Front ? VK_CULL_MODE_FRONT_BIT : VK_CULL_MODE_BACK_BIT;
}

void PipelineConfigInfo::SetWindingOrder(const WindingOrder winding)
{
	rasterizationInfo.frontFace = winding == WindingOrder::Clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
}

 void PipelineConfigInfo::SetColorWriteMask(const uint32_t mrtIndex, const ColorWriteMaskFlags mask)
 {
	ASSERT(mrtIndex < MaxRenderTargetCount, "mrtIndex must be lower then MaxRenderTargetCount");

 	colorBlendAttachment[mrtIndex].colorWriteMask = VkColorComponentFlags(mask);
 }
 
 void PipelineConfigInfo::SetColorWriteMask(const uint32_t mrtIndex, const uint32_t mrtCount, const ColorWriteMaskFlags masks[])
 {
	ASSERT(mrtIndex < MaxRenderTargetCount, "mrtIndex must be lower then MaxRenderTargetCount");

 	for (uint32_t i = 0; i < mrtCount; ++i)
 	{
 		colorBlendAttachment[mrtIndex].colorWriteMask = VkColorComponentFlags(masks[mrtIndex]);
 	}
 }

 PipelineConfigInfo::PipelineConfigInfo()
 {
	 vertexShader = nullptr;
	 pixelShader = nullptr;

	 inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

	 SetTopology(Topology::TriangleList);

	 viewport.x = 0.0f;
	 viewport.y = 0.0f;
	 viewport.width = static_cast<float>(Window::instance().GetExtent().width);
	 viewport.height = static_cast<float>(Window::instance().GetExtent().height);
	 viewport.minDepth = 0.0f;
	 viewport.maxDepth = 1.0f;

	 scissor.offset = { 0, 0 };
	 scissor.extent = { static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height) };

	 viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	 viewportInfo.viewportCount = 1;
	 viewportInfo.pViewports = &viewport;
	 viewportInfo.scissorCount = 1;
	 viewportInfo.pScissors = &scissor;

	 rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	 rasterizationInfo.depthClampEnable = VK_FALSE;
	 rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	 rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
	 rasterizationInfo.lineWidth = 1.0f;
	 SetCullMode(CullMode::Back);
	 SetWindingOrder(WindingOrder::Clockwise);
	 SetDepthBias(0.0f, 0.0f, 0.0f);

	 multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	 multisampleInfo.sampleShadingEnable = VK_FALSE;
	 multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	 multisampleInfo.minSampleShading = 1.0f;           // Optional
	 multisampleInfo.pSampleMask = nullptr;             // Optional
	 multisampleInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
	 multisampleInfo.alphaToOneEnable = VK_FALSE;       // Optional

	 colorBlendAttachment[0].colorWriteMask = 0xf;
	 colorBlendAttachment[0].blendEnable = VK_FALSE;

	 colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	 colorBlendInfo.logicOpEnable = VK_FALSE;
	 colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
	 colorBlendInfo.attachmentCount = 1;
	 colorBlendInfo.pAttachments = colorBlendAttachment;
	 colorBlendInfo.blendConstants[0] = 0.0f;  // Optional
	 colorBlendInfo.blendConstants[1] = 0.0f;  // Optional
	 colorBlendInfo.blendConstants[2] = 0.0f;  // Optional
	 colorBlendInfo.blendConstants[3] = 0.0f;  // Optional

	 depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	 depthStencilInfo.depthTestEnable = VK_TRUE;
	 depthStencilInfo.depthWriteEnable = VK_TRUE;
	 depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	 depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	 depthStencilInfo.minDepthBounds = 0.0f;  // Optional
	 depthStencilInfo.maxDepthBounds = 1.0f;  // Optional
	 depthStencilInfo.stencilTestEnable = VK_FALSE;
	 depthStencilInfo.front = {};  // Optional
	 depthStencilInfo.back = {};   // Optional
 }

void PipelineConfigInfo::SetAttachmentCount(const uint32_t attachmentCount)
{
	ASSERT(attachmentCount < MaxRenderTargetCount, "mrtIndex must be lower then MaxRenderTargetCount");

	colorBlendInfo.attachmentCount = attachmentCount;
}

void PipelineConfigInfo::SetAlphaBlendEnabled(const uint32_t mrtIndex, const bool enabled)
{
	ASSERT(mrtIndex < MaxRenderTargetCount, "mrtIndex must be lower then MaxRenderTargetCount");

	colorBlendAttachment[mrtIndex].blendEnable = enabled;
}

void PipelineConfigInfo::SetAlphaBlendFunction(const uint32_t mrtIndex, const BlendValue srcBlend, const BlendValue dstBlend)
{
	ASSERT(mrtIndex < MaxRenderTargetCount, "mrtIndex must be lower then MaxRenderTargetCount");

	colorBlendAttachment[mrtIndex].srcColorBlendFactor = GetVkBlendFactorFrom(srcBlend);
	colorBlendAttachment[mrtIndex].dstColorBlendFactor = GetVkBlendFactorFrom(dstBlend);
}

void PipelineConfigInfo::SetAlphaBlendFunction(const uint32_t mrtIndex, const BlendValue srcColorBlend,
	const BlendValue dstColorBlend, const BlendValue srcAlphaBlend, const BlendValue dstAlphablend)
{
	ASSERT(mrtIndex < MaxRenderTargetCount, "mrtIndex must be lower then MaxRenderTargetCount");

	colorBlendAttachment[mrtIndex].srcColorBlendFactor = GetVkBlendFactorFrom(srcColorBlend);
	colorBlendAttachment[mrtIndex].dstColorBlendFactor = GetVkBlendFactorFrom(dstColorBlend);
	colorBlendAttachment[mrtIndex].srcAlphaBlendFactor = GetVkBlendFactorFrom(srcAlphaBlend);
	colorBlendAttachment[mrtIndex].dstAlphaBlendFactor = GetVkBlendFactorFrom(dstAlphablend);
}

void PipelineConfigInfo::SetAlphaBlendOperation(const uint32_t mrtIndex, const BlendOperation colorOperation)
{
	ASSERT(mrtIndex < MaxRenderTargetCount, "mrtIndex must be lower then MaxRenderTargetCount");

	colorBlendAttachment[mrtIndex].colorBlendOp = GetVkBlendOpFrom(colorOperation);
}

void PipelineConfigInfo::SetAlphaBlendOperation(const uint32_t mrtIndex, const BlendOperation colorOperation, const BlendOperation alphaOperation)
{
	ASSERT(mrtIndex < MaxRenderTargetCount, "mrtIndex must be lower then MaxRenderTargetCount");

	colorBlendAttachment[mrtIndex].colorBlendOp = GetVkBlendOpFrom(colorOperation);
	colorBlendAttachment[mrtIndex].alphaBlendOp = GetVkBlendOpFrom(alphaOperation);
}

bool GraphicsPipelineKey::operator<(const GraphicsPipelineKey& k) const
{
	return memcmp(this, &k, sizeof(GraphicsPipelineKey)) < 0;
}

bool GraphicsPipelineKey::operator==(const GraphicsPipelineKey& k) const
{
	return memcmp(this, &k, sizeof(GraphicsPipelineKey)) == 0;
}

bool RenderPassKey::operator<(const RenderPassKey& k) const
{
	return memcmp(this, &k, sizeof(RenderPassKey)) < 0;

}

bool RenderPassKey::operator==(const RenderPassKey& k) const
{
	return memcmp(this, &k, sizeof(RenderPassKey)) == 0;
}


VulkanPipeline* PipelineManager::FindOrCreateGraphicsPipeline(VulkanDevice& device, PipelineConfigInfo& pipelineInfo)
{
	GraphicsPipelineKey key;

	key.m_VertexShaderCRC = pipelineInfo.vertexShader ? pipelineInfo.vertexShader->GetHash() : 0;
	key.m_PixelShaderCRC = pipelineInfo.pixelShader ? pipelineInfo.pixelShader->GetHash() : 0;
	key.m_Topology = pipelineInfo.inputAssemblyInfo.topology;
	key.m_Padding1 = 0;

	key.m_PolygonMode = pipelineInfo.rasterizationInfo.polygonMode;
	key.m_CullMode = pipelineInfo.rasterizationInfo.cullMode;
	key.m_Frontface = pipelineInfo.rasterizationInfo.frontFace;
	key.m_Padding2 = 0;

	//fill the key, find the pipeline in the map
	//if it is not in the map, add to map and create

	auto pipelineIterator = m_GraphicPipelines.find(key);
	if (pipelineIterator != m_GraphicPipelines.end())
	{
		return pipelineIterator->second;
	}
	else
	{
		VulkanPipeline* pipeline = new VulkanPipeline(device, pipelineInfo);
		//add to map
		m_GraphicPipelines[key] = pipeline;
		return pipeline;
	}
}

VulkanRenderPass* PipelineManager::FindOrCreateRenderPass(VulkanDevice& device, const std::vector<VulkanImageView*> renderTargets, const VulkanImageView* depthStencil, RenderPassConfigInfo& renderPassInfo)
{
    //create hash key
	RenderPassKey key{};

	for (size_t i = 0; i < renderTargets.size(); i++)
	{
		key.attachmentDescriptions[i].format = renderTargets[i]->GetFormat();
		key.attachmentDescriptions[i].samples = GetVkSampleFlagsFrom(renderTargets[i]->GetInfo().Resource->GetInfo().MultisampleType);
		key.attachmentDescriptions[i].loadOp = renderPassInfo.ClearRenderTargets ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		key.attachmentDescriptions[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		key.attachmentDescriptions[i].initialLayout = GetVkImageLayoutFrom(renderPassInfo.InitialRenderTargetState);
		key.attachmentDescriptions[i].finalLayout = GetVkImageLayoutFrom(renderPassInfo.FinalRenderTargetState);
	}
	
	if (depthStencil != nullptr)
	{
		key.depthStencilAttachmentDescription.format = depthStencil->GetFormat();
		key.depthStencilAttachmentDescription.samples = GetVkSampleFlagsFrom(depthStencil->GetInfo().Resource->GetInfo().MultisampleType);
		key.depthStencilAttachmentDescription.loadOp = renderPassInfo.ClearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		key.depthStencilAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		//missing for stencil, TODO: implement this if needed
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
		auto newRenderPass = new VulkanRenderPass(&device, renderTargets, depthStencil, renderPassInfo, "somename");
		m_RenderPasses[key] = newRenderPass;
		return newRenderPass;
	}

}

VulkanFramebuffer* PipelineManager::FindOrCreateFramebuffer(VulkanDevice& device, const std::vector<VulkanImageView*> renderTargets, const VulkanImageView* depthStencil, const VulkanRenderPass* renderpass, uint32_t width, uint32_t height)
{
	FramebufferKey key{};
	key.resize(5);

	for (size_t i = 0; i < renderTargets.size(); i++)
	{
		key[i] = renderTargets[i]->GetId();
	}
	if (depthStencil != nullptr)
	{
		key[4] = depthStencil->GetId();
	}

	auto framebuffer = m_Framebuffers.find(key);
	if (framebuffer != m_Framebuffers.end())
	{
		return framebuffer->second;
	}
	else
	{
		VulkanFramebufferInfo info{};
		info.height = height;
		info.width = width;

		auto newFramebuffer = new VulkanFramebuffer(&device, info, renderpass, renderTargets, depthStencil);
		m_Framebuffers[key] = newFramebuffer;

		return newFramebuffer;
	}
}

VulkanDescriptorSet* PipelineManager::FindOrCreateDescriptorSet(VulkanDevice& device, const VulkanDescriptorPool* desciptorPool, const VulkanDescriptorSetLayout* descriptorSetLayout, const std::vector<VulkanDescriptor*> descriptors)
{
	DescriptorSetKey key;

	for (size_t i = 0; i < descriptors.size(); i++)
	{
		switch (descriptors[i]->GetDescriptorInfo().Type)
		{
		case DescriptorType::UniformBuffer:
			key.push_back(descriptors[i]->GetDescriptorInfo().buffer->GetId());
			break;
		case DescriptorType::CombinedSampler:
			//only views now have unique ID so its a little bit hacky but who cares
			key.push_back(descriptors[i]->GetDescriptorInfo().combinedImageSampler->ImageView->GetId());
			break;
		}
	}

	auto descriptorset = m_DescriptorSets.find(key);
	if (descriptorset != m_DescriptorSets.end())
	{
		return descriptorset->second;
	}
	else
	{
		VulkanDescriptorSet* newDescSet = new VulkanDescriptorSet(&device, desciptorPool, descriptorSetLayout, descriptors, "descriptorset");
		m_DescriptorSets[key] = newDescSet;
		return newDescSet;
	}

	return nullptr;
}
