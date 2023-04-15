#pragma once

class VulkanPipeline;
class VulkanRenderPass;
struct UniformBufferObject;
class RenderPass;
struct RenderPassInfo;

typedef VkExtent2D Extent2D;

struct PushConstant
{
	PushConstant() : data(malloc(128)), size(0) {}
	~PushConstant() { free(data); }
	void* data;
	uint32_t size;
};

//keep in sync with UniformBufferObject in Renderer.h
enum class UBOElement : uint32_t
{
	ViewMatrix = 0,
	ProjectionMatrix = 4,
	CameraPosition = 8,
	DebugOption = 9,
	ViewProjInverse = 10,
	ViewProjMatrix = 14,
	PrevViewProjMatrix = 18,
	ViewInverse = 22,
	ProjInverse = 26,
	FrustrumInfo = 30,
	EyeXAxis = 31,
	EyeYAxis = 32,
	EyeZAxis = 33,
	ProjectionMatrixJittered = 34,
	CurrentFrameInfo = 38
};

class VulkanStateManager
{
public:
	VulkanStateManager();
	~VulkanStateManager();

	//pipeline
	PipelineInfo*		GetPipelineInfo() const { return m_PipelineInfo; }
	VulkanPipeline*		GetPipeline() const { return m_Pipeline; }
	void SetPipeline(VulkanPipeline* pipeline) { m_Pipeline = pipeline; m_DirtyPipeline = false; }
	bool GetPipelineDirty() { return m_DirtyPipeline; }
	void SetPipelineDirty(bool dirty) { m_DirtyPipeline = dirty; }

	void SetVertexShader(Shader* shader, std::string entryPoint = "main");
	void SetPixelShader(Shader* shader, std::string entryPoint = "main");
	void SetComputeShader(Shader* shader, std::string entryPoint = "main");
	void EnableWireframe(bool enable);
	void SetCullMode(const CullMode mode);
	void SetWindingOrder(const WindingOrder wo);

	//renderpass
	RenderPass*				GetRenderPass() const { return m_RenderPass; }
    RenderPassInfo* GetRenderPassInfo() { return m_RenderPassInfo; }
	void SetRenderPass(RenderPass* renderPass) { m_RenderPass = renderPass; m_DirtyRenderPass = false; }
	bool GetRenderPassDirty() { return m_DirtyRenderPass; }
	void SetRenderPassDirty(bool dirty) { m_DirtyRenderPass = dirty; }
	void SetRenderPassExtent(Extent2D extent);

	//descriptor set
	VulkanDescriptorSet* GetDescriptorSet() const { return m_DescriptorSet; }
	void SetDescriptorSet(VulkanDescriptorSet* descriptorSet) { m_DescriptorSet = descriptorSet; }
	bool GetDescriptorSetDirty() { return m_DirtyDescriptorSet; }
	void SetDescriptorSetDirty(bool dirty) { m_DirtyDescriptorSet = dirty; }

	//uniform buffer
	UniformBufferObject* GetUBO() const { return m_UBO; }
	void UpdateUBOElement(UBOElement element, uint32_t count, void* data);
	void SetUBODirty(bool dirty) { m_DirtyUBO = dirty; }
	bool GetUBODirty() { return m_DirtyUBO; }

	std::vector<VulkanImageView*>&  GetRenderTargets() { return m_RenderTargets; }
    VulkanImageView*                GetDepthStencil() const { return m_DepthStencil; }

	PushConstant* GetPushConst() { return &m_PushConst; }
	bool ShouldBindPushConst() const { return m_ShouldBindPushConst; }
	void SetShouldBindPushConst(bool should) { m_ShouldBindPushConst = should; }

	void SetRenderTarget(uint32_t slot, VulkanImageView* rt);
	void SetDepthStencil(VulkanImageView* ds);

	void ShouldCleanColor(LoadOp op);
	void ShouldCleanDepth(LoadOp op);

	void SetCombinedImageSampler(uint32_t slot, VulkanTexture* texture);
	void SetConstantBuffer(uint32_t slot, VulkanBuffer* buffer);
	void SetStorageImage(uint32_t slot, VulkanImageView* view);
	void SetStorageBuffer(uint32_t slot, VulkanBuffer* buffer);
	void SetSampledImage(uint32_t slot, VulkanImageView* view);
	void SetSampler(uint32_t slot, VulkanImageSampler* sampler);
#if defined(VULKAN_HWRT)
	void SetAccelerationStructure(uint32_t slot, RayTracing::AccelerationStructure* as);
#endif

	VulkanDescriptorSet* FinalizeDescriptorSet(VulkanDevice& device, const VulkanDescriptorPool* pool);
	uint8_t GetRenderTargetCount();
	bool HasDepthStencil() { return m_DepthStencil ? true : false; }

	PipelineType GetCurrentPipelineType() { return m_CurrentPipelinetype; }

	void Reset();
	void UpdateResourceStage(ManagableResource* texture);

private:
	PipelineInfo*			m_PipelineInfo;
	VulkanPipeline*		    m_Pipeline;
    RenderPassInfo*			m_RenderPassInfo;
	RenderPass*				m_RenderPass;
	UniformBufferObject*    m_UBO;

	std::vector<VulkanDescriptor*>	m_Descriptors;
	VulkanDescriptorSet*			m_DescriptorSet;

	bool m_DirtyPipeline = true;
	bool m_DirtyRenderPass = true;
	bool m_DirtyDescriptorSet = true;
	bool m_DirtyUBO = true;

	std::vector<VulkanImageView*>	m_RenderTargets;
    VulkanImageView*				m_DepthStencil = nullptr;

	PushConstant					m_PushConst;
	bool							m_ShouldBindPushConst = false;

	PipelineType m_CurrentPipelinetype;
};

