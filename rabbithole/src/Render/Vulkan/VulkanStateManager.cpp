#include "precomp.h"

#include "Render/PipelineManager.h"
#include "Render/Renderer.h"
#include "Render/SuperResolutionManager.h"

#define DEFAULT_UBO_ELEMENT_SIZE (uint32_t)16

VulkanStateManager::VulkanStateManager()
{
	m_Pipeline = nullptr;
	m_RenderPass = nullptr;
	m_Framebuffer = nullptr;
	m_PipelineConfig = new PipelineConfigInfo();
    m_RenderPassConfig = new RenderPassConfigInfo();
	m_CurrentPipelinetype = PipelineType::Count;

    VulkanRenderPass::DefaultRenderPassInfo(m_RenderPassConfig);

	m_DirtyPipeline = true;
	m_DirtyFramebuffer = true;
	m_DirtyRenderPass = true;
	m_DirtyUBO = false;

	SetFramebufferExtent(Extent2D{ 0, 0 });

    m_RenderTargets.reserve(MaxRenderTargetCount);

	m_UBO = new UniformBufferObject();
	memset(m_UBO, 0, sizeof(UniformBufferObject));
}

VulkanStateManager::~VulkanStateManager()
{
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

void VulkanStateManager::UpdateUBOElement(UBOElement element, uint32_t count, void* data)
{
	uint32_t sizeOfElement = DEFAULT_UBO_ELEMENT_SIZE * count;
	void* ubo = (char*)m_UBO + DEFAULT_UBO_ELEMENT_SIZE * (uint32_t)element;
	memcpy(ubo, data, sizeOfElement);
	m_DirtyUBO = true;
}

void VulkanStateManager::SetDepthStencil(VulkanImageView* ds)
{
    m_DepthStencil = ds;
    m_DirtyPipeline = true;
	m_DirtyRenderPass = true;
	m_DirtyFramebuffer = true;
}

void VulkanStateManager::ShouldCleanColor(bool clean)
{
    m_RenderPassConfig->ClearRenderTargets = clean;
	m_DirtyRenderPass = true;
}

void VulkanStateManager::ShouldCleanDepth(bool clean)
{
	m_RenderPassConfig->ClearDepth = clean;
	m_RenderPassConfig->ClearStencil = clean;
	m_DirtyRenderPass = true;
}

// for now descriptor key is vector of uint32_t with following layout:
// (slot, bufferId/imageviewId, type)
void VulkanStateManager::SetCombinedImageSampler(uint32_t slot, VulkanTexture* texture, uint32_t mipSlice)
{
	DescriptorKey k(3);
	k[0] = slot;
	k[1] = texture->GetView(mipSlice)->GetID();
	k[2] = (uint32_t)DescriptorType::CombinedSampler;

	//TODO: remove this ugly Singleton call
	auto& descriptorsMap = Renderer::instance().GetPipelineManager().GetDescriptors();
    auto descriptor = descriptorsMap.find(k);

    if (descriptor != descriptorsMap.end())
    {
        m_Descriptors.push_back(descriptor->second);
    }
    else
    {
		VulkanDescriptorInfo info{};
		info.Binding = slot;

		info.imageSampler = texture->GetSampler();
		info.imageView = texture->GetView(mipSlice);
		info.Type = DescriptorType::CombinedSampler;

		VulkanDescriptor* descriptor = new VulkanDescriptor(info);

        descriptorsMap[k] = descriptor;

		m_Descriptors.push_back(descriptor);
    }
}

void VulkanStateManager::SetConstantBuffer(uint32_t slot, VulkanBuffer* buffer)
{
	DescriptorKey k(3);
	k[0] = slot;
	k[1] = buffer->GetID();
	k[2] = (uint32_t)DescriptorType::UniformBuffer;

	//TODO: remove this ugly Singleton call
	auto& descriptorsMap = Renderer::instance().GetPipelineManager().GetDescriptors();
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

void VulkanStateManager::SetStorageImage(uint32_t slot, VulkanImageView* view)
{
	DescriptorKey k(3);
	k[0] = slot;
	k[1] = view->GetID();
	k[2] = (uint32_t)DescriptorType::StorageImage;

	//TODO: remove this ugly Singleton call
	auto& descriptorsMap = Renderer::instance().GetPipelineManager().GetDescriptors();
	auto descriptor = descriptorsMap.find(k);

	if (descriptor != descriptorsMap.end())
	{
		m_Descriptors.push_back(descriptor->second);
	}
	else
	{
		VulkanDescriptorInfo info{};
		info.Binding = slot;

		info.imageView = view;
		info.Type = DescriptorType::StorageImage;

		VulkanDescriptor* descriptor = new VulkanDescriptor(info);

		descriptorsMap[k] = descriptor;

		m_Descriptors.push_back(descriptor);
	}
}

void VulkanStateManager::SetStorageBuffer(uint32_t slot, VulkanBuffer* buffer)
{
	DescriptorKey k(3);
	k[0] = slot;
	k[1] = buffer->GetID();
	k[2] = (uint32_t)DescriptorType::StorageBuffer;

	//TODO: remove this ugly Singleton call
	auto& descriptorsMap = Renderer::instance().GetPipelineManager().GetDescriptors();
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
		info.Type = DescriptorType::StorageBuffer;

		VulkanDescriptor* descriptor = new VulkanDescriptor(info);

		descriptorsMap[k] = descriptor;

		m_Descriptors.push_back(descriptor);
	}
}

void VulkanStateManager::SetSampledImage(uint32_t slot, VulkanImageView* view)
{
	DescriptorKey k(3);
	k[0] = slot;
	k[1] = view->GetID();
	k[2] = (uint32_t)DescriptorType::SampledImage;

	//TODO: remove this ugly Singleton call
	auto& descriptorsMap = Renderer::instance().GetPipelineManager().GetDescriptors();
	auto descriptor = descriptorsMap.find(k);

	if (descriptor != descriptorsMap.end())
	{
		m_Descriptors.push_back(descriptor->second);
	}
	else
	{
		VulkanDescriptorInfo info{};
		info.Binding = slot;

		info.imageView= view;
		info.Type = DescriptorType::SampledImage;

		VulkanDescriptor* descriptor = new VulkanDescriptor(info);

		descriptorsMap[k] = descriptor;

		m_Descriptors.push_back(descriptor);
	}
}

void VulkanStateManager::SetSampler(uint32_t slot, VulkanImageSampler* sampler)
{
	DescriptorKey k(3);
	k[0] = slot;
	k[1] = sampler->GetID();
	k[2] = (uint32_t)DescriptorType::Sampler;

	//TODO: remove this ugly Singleton call
	auto& descriptorsMap = Renderer::instance().GetPipelineManager().GetDescriptors();
	auto descriptor = descriptorsMap.find(k);

	if (descriptor != descriptorsMap.end())
	{
		m_Descriptors.push_back(descriptor->second);
	}
	else
	{
		VulkanDescriptorInfo info{};
		info.Binding = slot;

		info.imageSampler = sampler;
		info.Type = DescriptorType::Sampler;

		VulkanDescriptor* descriptor = new VulkanDescriptor(info);

		descriptorsMap[k] = descriptor;

		m_Descriptors.push_back(descriptor);
	}
}

VulkanDescriptorSet* VulkanStateManager::FinalizeDescriptorSet(VulkanDevice& device, const VulkanDescriptorPool* pool)
{
	//TODO: remove this ugly Singleton call
	auto& pipelineManager = Renderer::instance().GetPipelineManager();
    VulkanDescriptorSet* descriptorset = pipelineManager.FindOrCreateDescriptorSet(device, pool, m_Pipeline->GetDescriptorSetLayout(), m_Descriptors);
    m_Descriptors.clear();
    return descriptorset;
}

uint8_t VulkanStateManager::GetRenderTargetCount()
{
    return static_cast<uint8_t>(m_RenderTargets.size());
}

void VulkanStateManager::UpdateResourceStage(ManagableResource* texture)
{
	texture->SetPreviousResourceStage(texture->GetCurrentResourceStage());

	ResourceStage currentStage = (m_CurrentPipelinetype != PipelineType::Count)
		? ((m_CurrentPipelinetype == PipelineType::Compute) ? ResourceStage::Compute : ResourceStage::Graphics)
		: ResourceStage::Count;

	texture->SetCurrentResourceStage(currentStage);
}

void VulkanStateManager::SetRenderTarget(uint32_t slot, VulkanImageView* rt)
{
	ASSERT(slot < MaxRenderTargetCount, "Reached out max number of RTs!");
	ASSERT(slot == m_RenderTargets.size(), "Slot must be current equal to current size!");
	m_RenderTargets.push_back(rt);

	m_DirtyFramebuffer = true;
	m_DirtyPipeline = true;
	m_DirtyRenderPass = true;
}

void VulkanStateManager::Reset()
{
	m_RenderTargets.clear();

    m_DepthStencil = nullptr;

    VulkanPipeline::DefaultPipelineConfigInfo(m_PipelineConfig, GetNativeWidth, GetNativeHeight);
    VulkanRenderPass::DefaultRenderPassInfo(m_RenderPassConfig);

	SetFramebufferExtent({ GetNativeWidth , GetNativeHeight });
}

void VulkanStateManager::SetVertexShader(Shader* shader, std::string entryPoint)
{
	m_CurrentPipelinetype = PipelineType::Graphics;
	m_PipelineConfig->vertexShader = shader;
	m_PipelineConfig->vsEntryPoint = entryPoint.c_str();
	m_DirtyPipeline = true;
}

void VulkanStateManager::SetPixelShader(Shader* shader, std::string entryPoint)
{
	m_CurrentPipelinetype = PipelineType::Graphics;
	m_PipelineConfig->pixelShader = shader;
	m_PipelineConfig->psEntryPoint = entryPoint.c_str();
	m_DirtyPipeline = true;
}

void VulkanStateManager::SetComputeShader(Shader* shader, std::string entryPoint)
{
	m_CurrentPipelinetype = PipelineType::Compute;
	m_PipelineConfig->computeShader = shader;
	m_PipelineConfig->csEntryPoint = entryPoint.c_str();
	m_DirtyPipeline = true;
}
