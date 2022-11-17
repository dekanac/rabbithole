#include "precomp.h"

#include "Render/Converters.h"
#include "Render/Model/Model.h"
#include "Render/Renderer.h"
#include "Render/Shader.h"
#include "Render/SuperResolutionManager.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <stdexcept>

VulkanPipeline::VulkanPipeline(VulkanDevice& device, PipelineInfo& pipelineInfo, PipelineType type)
	: m_VulkanDevice{ device } 
	, m_PipelineInfo(pipelineInfo)
	, m_RenderPass(pipelineInfo.renderPass)
	, m_Type(type)
{
	if (m_Type == PipelineType::Graphics)
	{
		CreateGraphicsPipeline();
	}
	if (m_Type == PipelineType::Compute)
	{
		CreateComputePipeline();
	}
}

VulkanPipeline::~VulkanPipeline() 
{
	vkDestroyPipeline(m_VulkanDevice.GetGraphicDevice(), m_Pipeline, nullptr);
	delete m_PipelineLayout;
	delete m_DescriptorSetLayout;
}

void VulkanPipeline::DefaultPipelineInfo(PipelineInfo*& pipelineInfo, uint32_t width, uint32_t height) 
{
	pipelineInfo->inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipelineInfo->SetTopology(Topology::TriangleList);

	pipelineInfo->viewport.x = 0.0f;
	pipelineInfo->viewport.y = 0.0f;
	pipelineInfo->viewport.width = static_cast<float>(width);
	pipelineInfo->viewport.height = static_cast<float>(height);
	pipelineInfo->viewport.minDepth = 0.0f;
	pipelineInfo->viewport.maxDepth = 1.0f;

	pipelineInfo->scissor.offset = { 0, 0 };
	pipelineInfo->scissor.extent = { width, height };

	pipelineInfo->viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipelineInfo->viewportInfo.viewportCount = 1;
	pipelineInfo->viewportInfo.pViewports = &pipelineInfo->viewport;
	pipelineInfo->viewportInfo.scissorCount = 1;
	pipelineInfo->viewportInfo.pScissors = &pipelineInfo->scissor;

	pipelineInfo->rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineInfo->rasterizationInfo.depthClampEnable = VK_FALSE;
	pipelineInfo->rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	pipelineInfo->rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
	pipelineInfo->rasterizationInfo.lineWidth = 1.0f;
	pipelineInfo->SetCullMode(CullMode::Back);
	pipelineInfo->SetWindingOrder(WindingOrder::Clockwise);
	pipelineInfo->SetDepthBias(0.0f, 0.0f, 0.0f);

	pipelineInfo->multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipelineInfo->multisampleInfo.sampleShadingEnable = VK_FALSE;
	pipelineInfo->multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	pipelineInfo->multisampleInfo.minSampleShading = 1.0f;           // Optional
	pipelineInfo->multisampleInfo.pSampleMask = nullptr;             // Optional
	pipelineInfo->multisampleInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
	pipelineInfo->multisampleInfo.alphaToOneEnable = VK_FALSE;       // Optional

	pipelineInfo->colorBlendAttachment[0].colorWriteMask = 0xf;
	pipelineInfo->colorBlendAttachment[0].blendEnable = VK_FALSE;

	pipelineInfo->colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pipelineInfo->colorBlendInfo.logicOpEnable = VK_FALSE;
	pipelineInfo->colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
	pipelineInfo->colorBlendInfo.attachmentCount = 1;
	pipelineInfo->colorBlendInfo.pAttachments = pipelineInfo->colorBlendAttachment;
	pipelineInfo->colorBlendInfo.blendConstants[0] = 0.0f;  // Optional
	pipelineInfo->colorBlendInfo.blendConstants[1] = 0.0f;  // Optional
	pipelineInfo->colorBlendInfo.blendConstants[2] = 0.0f;  // Optional
	pipelineInfo->colorBlendInfo.blendConstants[3] = 0.0f;  // Optional

	pipelineInfo->depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipelineInfo->depthStencilInfo.depthTestEnable = VK_TRUE;
	pipelineInfo->depthStencilInfo.depthWriteEnable = VK_TRUE;
	pipelineInfo->depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	pipelineInfo->depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	pipelineInfo->depthStencilInfo.minDepthBounds = 0.0f;  // Optional
	pipelineInfo->depthStencilInfo.maxDepthBounds = 1.0f;  // Optional
	pipelineInfo->depthStencilInfo.stencilTestEnable = VK_FALSE;
	pipelineInfo->depthStencilInfo.front = {};  // Optional
	pipelineInfo->depthStencilInfo.back = {};   // Optional
}

void VulkanPipeline::CreateComputePipeline()
{
	VkPipelineShaderStageCreateInfo computeShaderStage{};

	computeShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStage.stage = GetVkShaderStageFrom(m_PipelineInfo.computeShader->GetInfo().Type);
	computeShaderStage.module = m_PipelineInfo.computeShader->GetModule();
	computeShaderStage.pName = m_PipelineInfo.csEntryPoint.c_str();
	computeShaderStage.flags = 0;
	computeShaderStage.pNext = nullptr;

	m_DescriptorSetLayout = new VulkanDescriptorSetLayout(&m_VulkanDevice, { m_PipelineInfo.computeShader }, "Main");

	std::vector<VulkanDescriptorSetLayout*> descSetLayouts;
	descSetLayouts.push_back(m_DescriptorSetLayout);

	m_PipelineLayout = new VulkanPipelineLayout(m_VulkanDevice, descSetLayouts, m_PipelineInfo.computeShader->GetPushConstants());
	
	VkComputePipelineCreateInfo computePipelineInfo{};
	computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineInfo.stage = computeShaderStage;
	computePipelineInfo.layout = GET_VK_HANDLE_PTR(m_PipelineLayout);
	computePipelineInfo.flags = 0;

	VULKAN_API_CALL(vkCreateComputePipelines(m_VulkanDevice.GetGraphicDevice(), VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &m_Pipeline));

}

void VulkanPipeline::CreateGraphicsPipeline()
{
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};

	VkPipelineShaderStageCreateInfo vertexShaderStage{};

	vertexShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderStage.stage = GetVkShaderStageFrom(m_PipelineInfo.vertexShader->GetInfo().Type);
	vertexShaderStage.module = m_PipelineInfo.vertexShader->GetModule();
	vertexShaderStage.pName = m_PipelineInfo.vsEntryPoint.c_str();
	vertexShaderStage.flags = 0;
	vertexShaderStage.pNext = nullptr;

	shaderStages.push_back(vertexShaderStage);

	VkPipelineShaderStageCreateInfo pixelShaderStage{};

	pixelShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	pixelShaderStage.stage = GetVkShaderStageFrom(m_PipelineInfo.pixelShader->GetInfo().Type);
	pixelShaderStage.module = m_PipelineInfo.pixelShader->GetModule();
	pixelShaderStage.pName = m_PipelineInfo.psEntryPoint.c_str();
	pixelShaderStage.flags = 0;
	pixelShaderStage.pNext = nullptr;

	shaderStages.push_back(pixelShaderStage);

	m_DescriptorSetLayout = new VulkanDescriptorSetLayout(&m_VulkanDevice, { m_PipelineInfo.pixelShader, m_PipelineInfo.vertexShader }, "Main");
	
	std::vector<VulkanDescriptorSetLayout*> descSetLayouts;
	descSetLayouts.push_back(m_DescriptorSetLayout);

	std::vector<VkPushConstantRange> pushConsts{};
	pushConsts.insert(pushConsts.end(), m_PipelineInfo.vertexShader->GetPushConstants().begin(), m_PipelineInfo.vertexShader->GetPushConstants().end());
	pushConsts.insert(pushConsts.end(), m_PipelineInfo.pixelShader->GetPushConstants().begin(), m_PipelineInfo.pixelShader->GetPushConstants().end());

	m_PipelineLayout = new VulkanPipelineLayout(m_VulkanDevice, descSetLayouts, pushConsts);

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
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &m_PipelineInfo.inputAssemblyInfo;
	pipelineInfo.pViewportState = &m_PipelineInfo.viewportInfo;
	pipelineInfo.pRasterizationState = &m_PipelineInfo.rasterizationInfo;
	pipelineInfo.pMultisampleState = &m_PipelineInfo.multisampleInfo;
	pipelineInfo.pColorBlendState = &m_PipelineInfo.colorBlendInfo;
	pipelineInfo.pDepthStencilState = &m_PipelineInfo.depthStencilInfo;

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

	pipelineInfo.layout = GET_VK_HANDLE_PTR(m_PipelineLayout);
	pipelineInfo.renderPass = GET_VK_HANDLE_PTR(m_RenderPass);
	pipelineInfo.subpass = m_PipelineInfo.subpass;

	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VULKAN_API_CALL(vkCreateGraphicsPipelines(m_VulkanDevice.GetGraphicDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline));
}

// void PipelineInfo::SetVertexBinding(const VertexBinding* vertexBinding)
// {
// 	m_VertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
// 	m_VertexInputStateCreateInfo.pVertexBindingDescriptions = &get_st(vertexBinding)->m_VertexInputBindingDescription;
// 	m_VertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(get_st(vertexBinding)->m_VertexAttributeDescriptions.size());
// 	m_VertexInputStateCreateInfo.pVertexAttributeDescriptions = get_st(vertexBinding)->m_VertexAttributeDescriptions.data();
// }

void PipelineInfo::SetTopology(const Topology topology)
{
	inputAssemblyInfo.topology = GetVkPrimitiveTopologyFrom(topology);
}

void PipelineInfo::SetMultisampleType(const MultisampleType multiSampleType)
{
	multisampleInfo.rasterizationSamples = GetVkSampleFlagsFrom(multiSampleType);
	multisampleInfo.sampleShadingEnable = multiSampleType == MultisampleType::Sample_1 ? VK_FALSE : VK_TRUE;
}

void PipelineInfo::SetDepthWriteEnabled(const bool enabled)
{
	depthStencilInfo.depthWriteEnable = enabled;
}

void PipelineInfo::SetDepthTestEnabled(const bool enabled)
{
	depthStencilInfo.depthTestEnable = enabled;
}

void PipelineInfo::SetDepthCompare(const CompareOperation compareFunction)
{
	depthStencilInfo.depthCompareOp = GetVkCompareOperationFrom(compareFunction);
}

void PipelineInfo::SetDepthBias(const float depthBiasSlope, const float depthBias, const float depthBiasClamp)
{
	rasterizationInfo.depthBiasEnable = (depthBiasSlope != 0.0f) || (depthBias != 0.0f);
	rasterizationInfo.depthBiasSlopeFactor = depthBiasSlope;
	rasterizationInfo.depthBiasConstantFactor = depthBias;
	rasterizationInfo.depthBiasClamp = depthBiasClamp;
}

void PipelineInfo::SetStencilEnable(const bool enabled)
{
	depthStencilInfo.stencilTestEnable = enabled;
}

void PipelineInfo::SetStencilOperation(const StencilOperation stencilFail, const StencilOperation depthFail, const StencilOperation stencilPass)
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

void PipelineInfo::SetStencilFunction(const CompareOperation compareOperation, const uint8_t compareMask)
{
	VkCompareOp stencilCompareOp = GetVkCompareOperationFrom(compareOperation);

	depthStencilInfo.front.compareOp = stencilCompareOp;
	depthStencilInfo.back.compareOp = stencilCompareOp;

	depthStencilInfo.front.compareMask = compareMask;
	depthStencilInfo.back.compareMask = compareMask;
}

void PipelineInfo::SetStencilWriteMask(const uint8_t mask)
{
	depthStencilInfo.front.writeMask = mask;
	depthStencilInfo.back.writeMask = mask;
}

void PipelineInfo::SetWireFrameEnabled(const bool enable)
{
	rasterizationInfo.polygonMode = enable ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
}

void PipelineInfo::SetCullMode(const CullMode mode)
{
	rasterizationInfo.cullMode = mode == CullMode::None ? VK_CULL_MODE_NONE :
		mode == CullMode::Front ? VK_CULL_MODE_FRONT_BIT : VK_CULL_MODE_BACK_BIT;
}

void PipelineInfo::SetWindingOrder(const WindingOrder winding)
{
	rasterizationInfo.frontFace = winding == WindingOrder::Clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
}

 void PipelineInfo::SetColorWriteMask(const uint32_t mrtIndex, const ColorWriteMaskFlags mask)
 {
	ASSERT(mrtIndex < MaxRenderTargetCount, "mrtIndex must be lower then MaxRenderTargetCount");

 	colorBlendAttachment[mrtIndex].colorWriteMask = VkColorComponentFlags(mask);
 }
 
 void PipelineInfo::SetColorWriteMask(const uint32_t mrtIndex, const uint32_t mrtCount, const ColorWriteMaskFlags masks[])
 {
	ASSERT(mrtIndex < MaxRenderTargetCount, "mrtIndex must be lower then MaxRenderTargetCount");

 	for (uint32_t i = 0; i < mrtCount; ++i)
 	{
 		colorBlendAttachment[mrtIndex].colorWriteMask = VkColorComponentFlags(masks[mrtIndex]);
 	}
 }

 PipelineInfo::PipelineInfo()
 {
	 vertexShader = nullptr;
	 pixelShader = nullptr;
	 computeShader = nullptr;

	 inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

	 SetTopology(Topology::TriangleList);

	 viewport.x = 0.0f;
	 viewport.y = 0.0f;
	 viewport.width = static_cast<float>(GetNativeWidth);
	 viewport.height = static_cast<float>(GetNativeHeight);
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

void PipelineInfo::SetAttachmentCount(const uint32_t attachmentCount)
{
	ASSERT(attachmentCount <= MaxRenderTargetCount, "mrtIndex must be lower then MaxRenderTargetCount");

	colorBlendInfo.attachmentCount = attachmentCount;
}

void PipelineInfo::SetAlphaBlendEnabled(const uint32_t mrtIndex, const bool enabled)
{
	ASSERT(mrtIndex < MaxRenderTargetCount, "mrtIndex must be lower then MaxRenderTargetCount");

	colorBlendAttachment[mrtIndex].blendEnable = enabled;
}

void PipelineInfo::SetAlphaBlendFunction(const uint32_t mrtIndex, const BlendValue srcBlend, const BlendValue dstBlend)
{
	ASSERT(mrtIndex < MaxRenderTargetCount, "mrtIndex must be lower then MaxRenderTargetCount");

	colorBlendAttachment[mrtIndex].srcColorBlendFactor = GetVkBlendFactorFrom(srcBlend);
	colorBlendAttachment[mrtIndex].dstColorBlendFactor = GetVkBlendFactorFrom(dstBlend);
}

void PipelineInfo::SetAlphaBlendFunction(const uint32_t mrtIndex, const BlendValue srcColorBlend,
	const BlendValue dstColorBlend, const BlendValue srcAlphaBlend, const BlendValue dstAlphablend)
{
	ASSERT(mrtIndex < MaxRenderTargetCount, "mrtIndex must be lower then MaxRenderTargetCount");

	colorBlendAttachment[mrtIndex].srcColorBlendFactor = GetVkBlendFactorFrom(srcColorBlend);
	colorBlendAttachment[mrtIndex].dstColorBlendFactor = GetVkBlendFactorFrom(dstColorBlend);
	colorBlendAttachment[mrtIndex].srcAlphaBlendFactor = GetVkBlendFactorFrom(srcAlphaBlend);
	colorBlendAttachment[mrtIndex].dstAlphaBlendFactor = GetVkBlendFactorFrom(dstAlphablend);
}

void PipelineInfo::SetAlphaBlendOperation(const uint32_t mrtIndex, const BlendOperation colorOperation)
{
	ASSERT(mrtIndex < MaxRenderTargetCount, "mrtIndex must be lower then MaxRenderTargetCount");

	colorBlendAttachment[mrtIndex].colorBlendOp = GetVkBlendOpFrom(colorOperation);
}

void PipelineInfo::SetAlphaBlendOperation(const uint32_t mrtIndex, const BlendOperation colorOperation, const BlendOperation alphaOperation)
{
	ASSERT(mrtIndex < MaxRenderTargetCount, "mrtIndex must be lower then MaxRenderTargetCount");

	colorBlendAttachment[mrtIndex].colorBlendOp = GetVkBlendOpFrom(colorOperation);
	colorBlendAttachment[mrtIndex].alphaBlendOp = GetVkBlendOpFrom(alphaOperation);
}


VulkanPipelineLayout::VulkanPipelineLayout(VulkanDevice& device, std::vector<VulkanDescriptorSetLayout*>& descriptorSetLayouts, const std::vector<VkPushConstantRange>& pushConsts)
	: m_Device(device)
{
	std::vector<VkDescriptorSetLayout> vkDescriptorSetsLayouts;
	for (auto& descSetLayout : descriptorSetLayouts)
	{
		vkDescriptorSetsLayouts.push_back(*descSetLayout->GetLayout());
	}

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(vkDescriptorSetsLayouts.size());
	pipelineLayoutCreateInfo.pSetLayouts = vkDescriptorSetsLayouts.data();

	pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConsts.size());
	pipelineLayoutCreateInfo.pPushConstantRanges = pushConsts.data();

	VULKAN_API_CALL(vkCreatePipelineLayout(m_Device.GetGraphicDevice(), &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));
}

VulkanPipelineLayout::~VulkanPipelineLayout()
{
	vkDestroyPipelineLayout(m_Device.GetGraphicDevice(), m_PipelineLayout, nullptr);
}
