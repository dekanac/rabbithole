#pragma once

class VulkanPipeline;
class VulkanRenderPass;
class VulkanFramebuffer;
struct UniformBufferObject;

enum class UBOElement : uint32_t
{
	ViewMatrix = 0,
	ProjectionMatrix = 4,
	CameraPosition = 8,
};

class VulkanStateManager
{
public:
	VulkanStateManager();
	~VulkanStateManager();

	//pipeline
	PipelineConfigInfo* GetPipelineInfo() { return m_PipelineConfig; }
	VulkanPipeline*		GetPipeline() const { return m_Pipeline; }
	void SetPipeline(VulkanPipeline* pipeline) { m_Pipeline = pipeline; }
	bool GetPipelineDirty() { return m_DirtyPipeline; }
	void SetPipelineDirty(bool dirty) { m_DirtyPipeline = dirty; }

	void SetVertexShader(Shader* shader);
	void SetPixelShader(Shader* shader);
	void EnableWireframe(bool enable);
	void SetCullMode(const CullMode mode);

	//renderpass
	VulkanRenderPass*	GetRenderPass() const { return m_RenderPass; }
	void SetRenderPass(VulkanRenderPass* renderPass) { m_RenderPass = renderPass; }
	bool GetRenderPassDirty() { return m_DirtyRenderPass; }
	void SetRenderPassDirty(bool dirty) { m_DirtyRenderPass = dirty; }

	//framebuffer
	VulkanFramebuffer*	GetFramebuffer() const { return m_Framebuffer; }
	void SetFramebuffer(VulkanFramebuffer* framebuffer) { m_Framebuffer = framebuffer; }
	bool GetGramebufferDirty() { m_DirtyFramebuffer; }
	void SetFramebufferDirty(bool dirty) { m_DirtyFramebuffer = dirty; }

	//uniform buffer
	UniformBufferObject* GetUBO() const { return m_UBO; }
	void UpdateUBOElement(UBOElement element, uint32_t count, void* data);
	void SetUBODirty(bool dirty) { m_DirtyUBO = dirty; }
	bool GetUBODirty() { return m_DirtyUBO; }


private:
	PipelineConfigInfo* m_PipelineConfig;
	VulkanPipeline*		m_Pipeline;
	VulkanRenderPass*	m_RenderPass;
	VulkanFramebuffer*	m_Framebuffer;
	UniformBufferObject* m_UBO;

	bool m_DirtyPipeline;
	bool m_DirtyRenderPass;
	bool m_DirtyFramebuffer;
	bool m_DirtyUBO;

};

