#pragma once

class VulkanPipeline;
class VulkanRenderPass;
class VulkanFramebuffer;

class VulkanStateManager
{
public:
	VulkanStateManager();
	~VulkanStateManager();

	VulkanPipeline*		GetPipeline() const { return m_Pipeline; }
	VulkanRenderPass*	GetRenderPass() const { return m_RenderPass; }
	VulkanFramebuffer*	GetFramebuffer() const { return m_Framebuffer; }

	void SetPipeline(VulkanPipeline* pipeline) { m_Pipeline = pipeline; }
	void SetRenderPass(VulkanRenderPass* renderPass) { m_RenderPass = renderPass; }
	void SetFramebuffer(VulkanFramebuffer* framebuffer) { m_Framebuffer = framebuffer; }

private:
	VulkanPipeline*		m_Pipeline;
	VulkanRenderPass*	m_RenderPass;
	VulkanFramebuffer*	m_Framebuffer;

};