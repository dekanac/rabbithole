#pragma once

#include "VulkanDevice.h"
#include "VulkanPipeline.h"

#include <string>
#include <vector>

struct PipelineConfigInfo 
{
	PipelineConfigInfo(const PipelineConfigInfo&) = delete;
	PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

	VkViewport viewport;
	VkRect2D scissor;
	VkPipelineViewportStateCreateInfo viewportInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	VkPipelineRasterizationStateCreateInfo rasterizationInfo;
	VkPipelineMultisampleStateCreateInfo multisampleInfo;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo colorBlendInfo;
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
	VkPipelineLayout pipelineLayout = nullptr;
	VkRenderPass renderPass = nullptr;
	uint32_t subpass = 0;
};

class VulkanPipeline {
public:
	VulkanPipeline(
		VulkanDevice& device,
		const std::string& vertFilepath,
		const std::string& fragFilepath,
		const PipelineConfigInfo& configInfo);
	~VulkanPipeline();

	VulkanPipeline(const VulkanPipeline&) = delete;
	VulkanPipeline operator=(const VulkanPipeline&) = delete;

	void Bind(VkCommandBuffer commandBuffer);

	static void DefaultPipelineConfigInfo(
		PipelineConfigInfo& configInfo, uint32_t width, uint32_t height);

private:
	static std::vector<char> ReadFile(const std::string& filepath);

	void CreateGraphicsPipeline(
		const std::string& vertFilepath,
		const std::string& fragFilepath,
		const PipelineConfigInfo& configInfo);

	void CreateShaderModule(const std::vector<char>& code, ShaderType type, std::string name, const char* codeEntry);

	VulkanDevice& m_VulkanDevice;
	VkPipeline m_GraphicsPipeline;
	VkShaderModule m_VertShaderModule;
	VkShaderModule m_FragShaderModule;
};