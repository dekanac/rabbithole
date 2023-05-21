#include "precomp.h"

#include "Render/RenderPass.h"
#include "Render/PipelineManager.h"
#include "Render/Renderer.h"
#include "Render/SuperResolutionManager.h"

#define DEFAULT_UBO_ELEMENT_SIZE (uint32_t)16

VulkanStateManager::VulkanStateManager()
{
	m_Pipeline = nullptr;
	m_RenderPass = nullptr;
	m_PipelineInfo = new PipelineInfo();
	m_RenderPassInfo = new RenderPassInfo();
	m_CurrentPipelinetype = PipelineType::Count;

	m_DirtyPipeline = true;
	m_DirtyRenderPass = true;
	m_DirtyUBO = false;

    m_RenderTargets.reserve(MaxRenderTargetCount);

	m_UBO = new UniformBufferObject();
	memset(m_UBO, 0, sizeof(UniformBufferObject));
}

VulkanStateManager::~VulkanStateManager()
{
}

void VulkanStateManager::EnableWireframe(bool enable)
{
	m_PipelineInfo->SetWireFrameEnabled(enable);
	m_DirtyPipeline = true;
}

void VulkanStateManager::SetCullMode(const CullMode mode)
{
	m_PipelineInfo->SetCullMode(mode);
	m_DirtyPipeline = true;
}

void VulkanStateManager::SetWindingOrder(const WindingOrder wo)
{
    m_PipelineInfo->SetWindingOrder(wo);
    m_DirtyPipeline = true;
}

void VulkanStateManager::SetRenderPassExtent(Extent2D extent)
{
	m_RenderPassInfo->extent = extent;
	m_DirtyRenderPass = true;
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
}

void VulkanStateManager::ShouldCleanColor(LoadOp op)
{
	m_RenderPassInfo->ClearRenderTargets = op;
	m_DirtyRenderPass = true;
}

void VulkanStateManager::ShouldCleanDepth(LoadOp op)
{
	m_RenderPassInfo->ClearDepth = op;
	m_DirtyRenderPass = true;
}

// for now descriptor key is vector of uint32_t with following layout:
// (slot, bufferId/imageviewId, type)
void VulkanStateManager::SetCombinedImageSampler(uint32_t slot, VulkanTexture* texture)
{
	VulkanDescriptorInfo info{};
	info.Binding = slot;
	info.imageSampler = texture->GetSampler();
	info.imageView = texture->GetView();
	info.Type = DescriptorType::CombinedSampler;

	VulkanDescriptor descriptor(info);

	m_Descriptors.push_back(descriptor);
}

void VulkanStateManager::SetConstantBuffer(uint32_t slot, VulkanBuffer* buffer)
{
	VulkanDescriptorInfo info{};
	info.Binding = slot;
	info.buffer = buffer;
	info.Type = DescriptorType::UniformBuffer;

	VulkanDescriptor descriptor(info);

    m_Descriptors.push_back(descriptor);
}

void VulkanStateManager::SetStorageImage(uint32_t slot, VulkanImageView* view)
{
	VulkanDescriptorInfo info{};
	info.Binding = slot;
	info.imageView = view;
	info.Type = DescriptorType::StorageImage;

	VulkanDescriptor descriptor(info);

	m_Descriptors.push_back(descriptor);
}

void VulkanStateManager::SetStorageBuffer(uint32_t slot, VulkanBuffer* buffer)
{
	VulkanDescriptorInfo info{};
	info.Binding = slot;
	info.buffer = buffer;
	info.Type = DescriptorType::StorageBuffer;

	VulkanDescriptor descriptor(info);

	m_Descriptors.push_back(descriptor);
}

void VulkanStateManager::SetSampledImage(uint32_t slot, VulkanImageView* view)
{
	VulkanDescriptorInfo info{};
	info.Binding = slot;
	info.imageView= view;
	info.Type = DescriptorType::SampledImage;

	VulkanDescriptor descriptor(info);

	m_Descriptors.push_back(descriptor);
}

void VulkanStateManager::SetSampler(uint32_t slot, VulkanImageSampler* sampler)
{
	VulkanDescriptorInfo info{};
	info.Binding = slot;
	info.imageSampler = sampler;
	info.Type = DescriptorType::Sampler;
		
	VulkanDescriptor descriptor(info);

	m_Descriptors.push_back(descriptor);
}

#if defined(VULKAN_HWRT)
void VulkanStateManager::SetAccelerationStructure(uint32_t slot, RayTracing::AccelerationStructure* as)
{
	VulkanDescriptorInfo info{};
	info.Binding = slot;
	info.accelerationStructure = as;
	info.Type = DescriptorType::AccelerationStructure;

	VulkanDescriptor descriptor(info);

	m_Descriptors.push_back(descriptor);
}
#endif

VulkanDescriptorSet* VulkanStateManager::FinalizeDescriptorSet(VulkanDevice& device, const VulkanDescriptorPool* pool)
{
	PipelineManager& pipelineManager = Renderer::instance().GetPipelineManager();
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
	texture->SetCurrentResourceStage(GetResourceStageFrom(m_CurrentPipelinetype));
}

void VulkanStateManager::SetRenderTarget(uint32_t slot, VulkanImageView* rt)
{
	ASSERT(slot < MaxRenderTargetCount, "Reached out max number of RTs!");
	ASSERT(slot == m_RenderTargets.size(), "Slot must be current equal to current size!");
	m_RenderTargets.push_back(rt);

	m_DirtyPipeline = true;
	m_DirtyRenderPass = true;
}

void VulkanStateManager::Reset()
{
	m_RenderTargets.clear();
	m_Descriptors.clear();

    m_DepthStencil = nullptr;

    VulkanPipeline::DefaultPipelineInfo(m_PipelineInfo, GetNativeWidth, GetNativeHeight);
	RenderPass::DefaultRenderPassInfo(*m_RenderPassInfo, GetNativeWidth, GetNativeHeight);
}

void VulkanStateManager::SetVertexShader(Shader* shader, std::string entryPoint)
{
	m_CurrentPipelinetype = PipelineType::Graphics;
	m_PipelineInfo->vertexShader = shader;
	m_PipelineInfo->vsEntryPoint = entryPoint.c_str();
	m_DirtyPipeline = true;
}

void VulkanStateManager::SetPixelShader(Shader* shader, std::string entryPoint)
{
	m_CurrentPipelinetype = PipelineType::Graphics;
	m_PipelineInfo->pixelShader = shader;
	m_PipelineInfo->psEntryPoint = entryPoint.c_str();
	m_DirtyPipeline = true;
}

void VulkanStateManager::SetComputeShader(Shader* shader, std::string entryPoint)
{
	m_CurrentPipelinetype = PipelineType::Compute;
	m_PipelineInfo->computeShader = shader;
	m_PipelineInfo->csEntryPoint = entryPoint.c_str();
	m_DirtyPipeline = true;
}

void VulkanStateManager::SetRayTracingShaders(std::array<Shader*, MaxRTShaders> rtShaders)
{
	m_CurrentPipelinetype = PipelineType::RayTracing;
	m_PipelineInfo->rayTracingShaders = rtShaders;
	m_DirtyPipeline = true;
}
