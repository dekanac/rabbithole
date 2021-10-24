#include "precomp.h"

#include "Render/Renderer.h"

#define DEFAULT_UBO_ELEMENT_SIZE (uint32_t)16

VulkanStateManager::VulkanStateManager()
{
	m_Pipeline = nullptr;
	m_RenderPass = nullptr;
	m_Framebuffer = nullptr;
	m_PipelineConfig = new PipelineConfigInfo();
	VulkanPipeline::DefaultPipelineConfigInfo(m_PipelineConfig, 1280, 720);

	m_DirtyPipeline = false; 
	m_DirtyUBO = false;

	m_UBO = new UniformBufferObject();
	memset(m_UBO, 0, sizeof(UniformBufferObject));
}

VulkanStateManager::~VulkanStateManager()
{
	delete m_RenderPass;
	delete m_Pipeline;
	delete m_Framebuffer;
	delete m_UBO;
}

void VulkanStateManager::EnableWireframe(bool enable)
{
	m_PipelineConfig->SetWireFrameEnabled(enable);
	m_DirtyPipeline = true;
}

void VulkanStateManager::SetCullMode(const CullMode mode)
{
	m_PipelineConfig->SetCullMode(mode);
	m_DirtyPipeline = true;
}

void VulkanStateManager::UpdateUBOElement(UBOElement element, uint32_t count, void* data)
{
	uint32_t sizeOfElement = DEFAULT_UBO_ELEMENT_SIZE * count;
	void* ubo = (char*)m_UBO + DEFAULT_UBO_ELEMENT_SIZE * (uint32_t)element;
	memcpy(ubo, data, sizeOfElement);
	m_DirtyUBO = true;
}

void VulkanStateManager::SetVertexShader(Shader* shader)
{
	m_PipelineConfig->vertexShader = shader;
	m_DirtyPipeline = true;
}

void VulkanStateManager::SetPixelShader(Shader* shader)
{
	m_PipelineConfig->pixelShader = shader;
	m_DirtyPipeline = true;
}
