#pragma once

#include "Renderer.h"

template<>
void Renderer::BindPipeline<GraphicsPipeline>()
{
	RSTManager.CommitBarriers();

	//renderpass
	auto& attachments = m_StateManager->GetRenderTargets();
	auto depthStencil = m_StateManager->GetDepthStencil();
	auto renderPassInfo = m_StateManager->GetRenderPassInfo();

	VulkanRenderPass* renderpass =
		m_StateManager->GetRenderPassDirty()
		? PipelineManager::instance().FindOrCreateRenderPass(m_VulkanDevice, attachments, depthStencil, *renderPassInfo)
		: m_StateManager->GetRenderPass();

	m_StateManager->SetRenderPass(renderpass);

	//framebuffer
	VulkanFramebufferInfo framebufferInfo{};
	framebufferInfo.width = m_StateManager->GetFramebufferExtent().width;
	framebufferInfo.height = m_StateManager->GetFramebufferExtent().height;
	
	VulkanFramebuffer* framebuffer =
		m_StateManager->GetFramebufferDirty()
		? PipelineManager::instance().FindOrCreateFramebuffer(m_VulkanDevice, attachments, depthStencil, renderpass, framebufferInfo)
		: m_StateManager->GetFramebuffer();

	m_StateManager->SetFramebuffer(framebuffer);

	//pipeline
	auto pipelineInfo = m_StateManager->GetPipelineInfo();

	pipelineInfo->renderPass = m_StateManager->GetRenderPass();
	auto pipeline =
		m_StateManager->GetPipelineDirty()
		? PipelineManager::instance().FindOrCreateGraphicsPipeline(m_VulkanDevice, *pipelineInfo)
		: m_StateManager->GetPipeline();

	m_StateManager->SetPipeline(pipeline);

	pipeline->Bind(GetCurrentCommandBuffer());

	BindDescriptorSets();
}

template<>
void Renderer::BindPipeline<ComputePipeline>()
{
	RSTManager.CommitBarriers();

	auto pipelineInfo = m_StateManager->GetPipelineInfo();

	VulkanPipeline* computePipeline = m_StateManager->GetPipelineDirty()
		? PipelineManager::instance().FindOrCreateComputePipeline(m_VulkanDevice, *pipelineInfo)
		: m_StateManager->GetPipeline();

	m_StateManager->SetPipeline(computePipeline);

	computePipeline->Bind(GetCurrentCommandBuffer());

	BindDescriptorSets();
}