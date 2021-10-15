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

void VulkanPipeline::DefaultPipelineConfigInfo(PipelineConfigInfo& configInfo, uint32_t width, uint32_t height) 
{

	configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	configInfo.SetTopology(Topology::TriangleList);

	configInfo.viewport.x = 0.0f;
	configInfo.viewport.y = 0.0f;
	configInfo.viewport.width = static_cast<float>(width);
	configInfo.viewport.height = static_cast<float>(height);
	configInfo.viewport.minDepth = 0.0f;
	configInfo.viewport.maxDepth = 1.0f;

	configInfo.scissor.offset = { 0, 0 };
	configInfo.scissor.extent = { width, height };

	configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	configInfo.viewportInfo.viewportCount = 1;
	configInfo.viewportInfo.pViewports = &configInfo.viewport;
	configInfo.viewportInfo.scissorCount = 1;
	configInfo.viewportInfo.pScissors = &configInfo.scissor;

	configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
	configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
	configInfo.rasterizationInfo.lineWidth = 1.0f;
	configInfo.SetCullMode(CullMode::Back);
	configInfo.SetWindingOrder(WindingOrder::Clockwise);
	configInfo.SetDepthBias(0.0f, 0.0f, 0.0f);

	configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
	configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	configInfo.multisampleInfo.minSampleShading = 1.0f;           // Optional
	configInfo.multisampleInfo.pSampleMask = nullptr;             // Optional
	configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
	configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;       // Optional

	configInfo.colorBlendAttachment[0].colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	configInfo.colorBlendAttachment[0].blendEnable = VK_FALSE;
	configInfo.colorBlendAttachment[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
	configInfo.colorBlendAttachment[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
	configInfo.colorBlendAttachment[0].colorBlendOp = VK_BLEND_OP_ADD;              // Optional
	configInfo.colorBlendAttachment[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
	configInfo.colorBlendAttachment[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
	configInfo.colorBlendAttachment[0].alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

	configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
	configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
	configInfo.colorBlendInfo.attachmentCount = 1;
	configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment[0];
	configInfo.colorBlendInfo.blendConstants[0] = 0.0f;  // Optional
	configInfo.colorBlendInfo.blendConstants[1] = 0.0f;  // Optional
	configInfo.colorBlendInfo.blendConstants[2] = 0.0f;  // Optional
	configInfo.colorBlendInfo.blendConstants[3] = 0.0f;  // Optional

	configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
	configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
	configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	configInfo.depthStencilInfo.minDepthBounds = 0.0f;  // Optional
	configInfo.depthStencilInfo.maxDepthBounds = 1.0f;  // Optional
	configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
	configInfo.depthStencilInfo.front = {};  // Optional
	configInfo.depthStencilInfo.back = {};   // Optional
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
	pipelineInfo.pDynamicState = nullptr;

	pipelineInfo.layout = *(m_DescriptorSetLayout->GetPipelineLayout());
	pipelineInfo.renderPass = m_RenderPass->GetRenderPass();
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

	 colorBlendAttachment[0].colorWriteMask =
		 VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
		 VK_COLOR_COMPONENT_A_BIT;
	 colorBlendAttachment[0].blendEnable = VK_FALSE;
	 colorBlendAttachment[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
	 colorBlendAttachment[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
	 colorBlendAttachment[0].colorBlendOp = VK_BLEND_OP_ADD;              // Optional
	 colorBlendAttachment[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
	 colorBlendAttachment[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
	 colorBlendAttachment[0].alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

	 colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	 colorBlendInfo.logicOpEnable = VK_FALSE;
	 colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
	 colorBlendInfo.attachmentCount = 1;
	 colorBlendInfo.pAttachments = &colorBlendAttachment[0];
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

VulkanPipeline* PipelineManager::FindOrCreateGraphicsPipeline(const GraphicPipelineDescription& description)
{
	GraphicsPipelineKey key;

	//TODO: fill this properly please
	return nullptr;
}
