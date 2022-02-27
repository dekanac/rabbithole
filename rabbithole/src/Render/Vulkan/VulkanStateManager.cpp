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
    //dont need this line because the values are already set in constructor
	//VulkanPipeline::DefaultPipelineConfigInfo(m_PipelineConfig, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    VulkanRenderPass::DefaultRenderPassInfo(m_RenderPassConfig);

	m_DirtyPipeline = false; 
	m_DirtyUBO = false;

    //m_DescriptorSetManager = new DescriptorSetManager();
    m_RenderTargets.resize(MaxRenderTargetCount);

	m_UBO = new UniformBufferObject();
	memset(m_UBO, 0, sizeof(UniformBufferObject));
}

VulkanStateManager::~VulkanStateManager()
{
	delete m_RenderPass;
	delete m_Pipeline;
	delete m_Framebuffer;
	delete m_UBO;
    //delete m_DescriptorSetManager;
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

void VulkanStateManager::SetWindingOrder(const WindingOrder wo)
{
    m_PipelineConfig->SetWindingOrder(wo);
    m_DirtyPipeline = true;
}

void VulkanStateManager::SetViewport(float x, float y, float width, float height)
{
	m_PipelineConfig->viewport.x = x;
	m_PipelineConfig->viewport.y = y;
	m_PipelineConfig->viewport.width = width;
	m_PipelineConfig->viewport.height = height;

    m_DirtyPipeline = true;
}

void VulkanStateManager::UpdateUBOElement(UBOElement element, uint32_t count, void* data)
{
	uint32_t sizeOfElement = DEFAULT_UBO_ELEMENT_SIZE * count;
	void* ubo = (char*)m_UBO + DEFAULT_UBO_ELEMENT_SIZE * (uint32_t)element;
	memcpy(ubo, data, sizeOfElement);
	m_DirtyUBO = true;
}

std::vector<VulkanImageView*>& VulkanStateManager::GetRenderTargets()
{
    m_RenderTargets.resize(GetRenderTargetCount());

    if (m_RenderTarget0 != nullptr)
    {
        m_RenderTargets[0] = m_RenderTarget0;

        if (m_RenderTarget1 != nullptr)
        {
            m_RenderTargets[1] = m_RenderTarget1;
        
            if (m_RenderTarget2 != nullptr)
            {
                m_RenderTargets[2] = m_RenderTarget2;

                if (m_RenderTarget3 != nullptr)
                {
                    m_RenderTargets[3] = m_RenderTarget3;
                }
            }
        }
    }

    return m_RenderTargets;
}

void VulkanStateManager::SetRenderTarget0(VulkanImageView* rt)
{
    m_RenderTarget0 = rt;
    m_DirtyPipeline = true;
}

void VulkanStateManager::SetRenderTarget1(VulkanImageView* rt)
{
    m_RenderTarget1 = rt;
    m_DirtyPipeline = true;
}

void VulkanStateManager::SetRenderTarget2(VulkanImageView* rt)
{
    m_RenderTarget2 = rt;
    m_DirtyPipeline = true;
}

void VulkanStateManager::SetRenderTarget3(VulkanImageView* rt)
{
    m_RenderTarget3 = rt;
    m_DirtyPipeline = true;
}

void VulkanStateManager::SetDepthStencil(VulkanImageView* ds)
{
    m_DepthStencil = ds;
    m_DirtyPipeline = true;
}

void VulkanStateManager::ShouldCleanColor(bool clean)
{
    m_RenderPassConfig->ClearRenderTargets = clean;
    m_DirtyPipeline = true;
}

void VulkanStateManager::ShouldCleanDepth(bool clean)
{
	m_RenderPassConfig->ClearDepth = clean;
	m_RenderPassConfig->ClearStencil = clean;
    m_DirtyPipeline = true;
}

void VulkanStateManager::SetCombinedImageSampler(uint32_t slot, VulkanTexture* texture)
{
	//for now descriptor key is vector of uint32_t with following layout:
	// (slot, bufferId/imageviewId/type)
	DescriptorKey k(3);
	k[0] = slot;
	k[1] = texture->GetView()->GetID();
	k[2] = (uint32_t)DescriptorType::CombinedSampler;

    auto& descriptorsMap = PipelineManager::instance().m_Descriptors;
    auto descriptor = descriptorsMap.find(k);

    if (descriptor != descriptorsMap.end())
    {
        m_Descriptors.push_back(descriptor->second);
    }
    else
    {
		//TODO: dual implementation, get rid of one
		VulkanDescriptorInfo info{};
		info.Binding = slot;

		//probably memory leak TODO: see whats going on here
		info.combinedImageSampler = new CombinedImageSampler();
		info.combinedImageSampler->ImageSampler = texture->GetSampler();
		info.combinedImageSampler->ImageView = texture->GetView();
		info.Type = DescriptorType::CombinedSampler;

		VulkanDescriptor* descriptor = new VulkanDescriptor(info);

        descriptorsMap[k] = descriptor;

		m_Descriptors.push_back(descriptor);
    }
}

void VulkanStateManager::SetConstantBuffer(uint32_t slot, VulkanBuffer* buffer, uint64_t offset, uint64_t range)
{
	//for now descriptor key is vector of uint32_t with following layout:
    // (slot, bufferId/imageviewId/type)
	DescriptorKey k(3);
	k[0] = slot;
	k[1] = buffer->GetID();
	k[2] = (uint32_t)DescriptorType::UniformBuffer;


	auto& descriptorsMap = PipelineManager::instance().m_Descriptors;
	auto descriptor = descriptorsMap.find(k);

	if (descriptor != descriptorsMap.end())
	{
		m_Descriptors.push_back(descriptor->second);
	}
    else
    {
	    VulkanDescriptorInfo info{};
	    info.Binding = slot;
	    info.buffer = buffer;
	    info.Type = DescriptorType::UniformBuffer;

	    VulkanDescriptor* descriptor = new VulkanDescriptor(info);

	    descriptorsMap[k] = descriptor;

        m_Descriptors.push_back(descriptor);
    }
}

void VulkanStateManager::SetStorageImage(uint32_t slot, VulkanTexture* texture)
{
	//for now descriptor key is vector of uint32_t with following layout:
	// (slot, bufferId/imageviewId/type)
	DescriptorKey k(3);
	k[0] = slot;
	k[1] = texture->GetView()->GetID();
	k[2] = (uint32_t)DescriptorType::StorageImage;

	auto& descriptorsMap = PipelineManager::instance().m_Descriptors;
	auto descriptor = descriptorsMap.find(k);

	if (descriptor != descriptorsMap.end())
	{
		m_Descriptors.push_back(descriptor->second);
	}
	else
	{
		VulkanDescriptorInfo info{};
		info.Binding = slot;

		info.imageView = texture->GetView();
		info.Type = DescriptorType::StorageImage;

		VulkanDescriptor* descriptor = new VulkanDescriptor(info);

		descriptorsMap[k] = descriptor;

		m_Descriptors.push_back(descriptor);
	}
}

VulkanDescriptorSet* VulkanStateManager::FinalizeDescriptorSet(VulkanDevice& device, const VulkanDescriptorPool* pool)
{
    VulkanDescriptorSet* descriptorset = PipelineManager::instance().FindOrCreateDescriptorSet(device, pool, m_Pipeline->GetDescriptorSetLayout(), m_Descriptors);
    m_Descriptors.clear();
    return descriptorset;
}

uint8_t VulkanStateManager::GetRenderTargetCount()
{
    //man, this line makes me proud :'D
    return m_RenderTarget0 ? (m_RenderTarget1 ? (m_RenderTarget2 ? (m_RenderTarget3 ? 4 : 3) : 2) : 1) : 0;
}

void VulkanStateManager::Reset()
{
    m_RenderTarget0 = nullptr;
    m_RenderTarget1 = nullptr;
	m_RenderTarget2 = nullptr;
	m_RenderTarget3 = nullptr;

    m_DepthStencil = nullptr;

    VulkanPipeline::DefaultPipelineConfigInfo(m_PipelineConfig, Window::instance().GetExtent().width, Window::instance().GetExtent().height);
    VulkanRenderPass::DefaultRenderPassInfo(m_RenderPassConfig);
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

void VulkanStateManager::SetComputeShader(Shader* shader)
{
	m_PipelineConfig->computeShader = shader;
	m_DirtyPipeline = true;
}
