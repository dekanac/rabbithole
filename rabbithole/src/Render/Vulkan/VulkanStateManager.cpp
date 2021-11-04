#include "precomp.h"

#include "Render/Renderer.h"

#define DEFAULT_UBO_ELEMENT_SIZE (uint32_t)16

VulkanStateManager::VulkanStateManager()
{
	m_Pipeline = nullptr;
	m_RenderPass = nullptr;
	m_Framebuffer = nullptr;
	m_PipelineConfig = new PipelineConfigInfo();
    m_RenderPassConfig = new RenderPassConfigInfo();
	VulkanPipeline::DefaultPipelineConfigInfo(m_PipelineConfig, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    VulkanRenderPass::DefaultRenderPassInfo(m_RenderPassConfig);

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

std::vector<VulkanImageView*> VulkanStateManager::GetRenderTargets() const
{
    std::vector<VulkanImageView*> renderTargets;

    if (m_RenderTarget0 != nullptr)
    {
        renderTargets.push_back(m_RenderTarget0);

        if (m_RenderTarget1 != nullptr)
        {
            renderTargets.push_back(m_RenderTarget1);
        
            if (m_RenderTarget2 != nullptr)
            {
                renderTargets.push_back(m_RenderTarget2);

                if (m_RenderTarget3 != nullptr)
                {
                    renderTargets.push_back(m_RenderTarget3);
                }
            }
        }
    }

    return renderTargets;
}

void VulkanStateManager::SetRenderTarget0(VulkanImageView* rt)
{
    m_RenderTarget0 = rt;
    m_DirtyRenderPass = true;
}

void VulkanStateManager::SetRenderTarget1(VulkanImageView* rt)
{
    m_RenderTarget1 = rt;
    m_DirtyRenderPass = true;
}

void VulkanStateManager::SetRenderTarget2(VulkanImageView* rt)
{
    m_RenderTarget2 = rt;
    m_DirtyRenderPass = true;
}

void VulkanStateManager::SetRenderTarget3(VulkanImageView* rt)
{
    m_RenderTarget3 = rt;
    m_DirtyRenderPass = true;
}

void VulkanStateManager::SetDepthStencil(VulkanImageView* ds)
{
    m_DepthStencil = ds;
    m_DirtyRenderPass = true;
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
