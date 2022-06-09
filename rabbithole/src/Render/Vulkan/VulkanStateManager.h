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
	DebugOption = 9,
	ViewProjInverse = 10,
	ViewProjMatrix = 14,
	PrevViewProjMatrix = 18
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
	void SetComputeShader(Shader* shader);
	void EnableWireframe(bool enable);
	void SetCullMode(const CullMode mode);
	void SetWindingOrder(const WindingOrder wo);

	//renderpass
	VulkanRenderPass*	    GetRenderPass() const { return m_RenderPass; }
    RenderPassConfigInfo*   GetRenderPassInfo() { return m_RenderPassConfig; }
	void SetRenderPass(VulkanRenderPass* renderPass) { m_RenderPass = renderPass; }
	bool GetRenderPassDirty() { return m_DirtyRenderPass; }
	void SetRenderPassDirty(bool dirty) { m_DirtyRenderPass = dirty; }

	//framebuffer
	VulkanFramebuffer*	GetFramebuffer() const { return m_Framebuffer; }
	void SetFramebuffer(VulkanFramebuffer* framebuffer) { m_Framebuffer = framebuffer; }
	bool GetGramebufferDirty() { return m_DirtyFramebuffer; }
	void SetFramebufferDirty(bool dirty) { m_DirtyFramebuffer = dirty; }

	//descriptor set
	VulkanDescriptorSet* GetDescriptorSet() const { return m_DescriptorSet; }
	void SetDescriptorSet(VulkanDescriptorSet* descriptorSet) { m_DescriptorSet = descriptorSet; }
	bool GetDescriptorSetDirty() { return m_DirtyDescriptorSet; }
	void SetDescriptorSetDirty(bool dirty) { m_DirtyDescriptorSet = dirty; }

	DescriptorSetManager* GetDescriptorSetManager() { return m_DescriptorSetManager; }

	//uniform buffer
	UniformBufferObject* GetUBO() const { return m_UBO; }
	void UpdateUBOElement(UBOElement element, uint32_t count, void* data);
	void SetUBODirty(bool dirty) { m_DirtyUBO = dirty; }
	bool GetUBODirty() { return m_DirtyUBO; }

    std::vector<VulkanImageView*>&  GetRenderTargets();
    VulkanImageView*                GetDepthStencil() const { return m_DepthStencil; }

    void SetRenderTarget0(VulkanImageView* rt);
    void SetRenderTarget1(VulkanImageView* rt);
    void SetRenderTarget2(VulkanImageView* rt);
    void SetRenderTarget3(VulkanImageView* rt);
	void SetRenderTarget4(VulkanImageView* rt);
	void SetDepthStencil(VulkanImageView* ds);

	void ShouldCleanColor(bool clean);
	void ShouldCleanDepth(bool clean);

	void SetCombinedImageSampler(uint32_t slot, VulkanTexture* texture);
	void SetConstantBuffer(uint32_t slot, VulkanBuffer* buffer);
	void SetStorageImage(uint32_t slot, VulkanTexture* texture);
	void SetStorageBuffer(uint32_t slot, VulkanBuffer* buffer);
	void SetSampledImage(uint32_t slot, VulkanTexture* texture);
	void SetSampler(uint32_t slot, VulkanTexture* texture);
	void SetSampler(uint32_t slot, VulkanImageSampler* sampler);

	VulkanDescriptorSet* FinalizeDescriptorSet(VulkanDevice& device, const VulkanDescriptorPool* pool);
	uint8_t GetRenderTargetCount();
	bool HasDepthStencil() { return m_DepthStencil ? true : false; }

	PipelineType GetCurrentPipelineType() { return m_CurrentPipelinetype; }

	void Reset();
	void UpdateResourceStage(VulkanTexture* texture);

private:
	PipelineConfigInfo*     m_PipelineConfig;
	VulkanPipeline*		    m_Pipeline;
    RenderPassConfigInfo*   m_RenderPassConfig;
	VulkanRenderPass*	    m_RenderPass;
	VulkanFramebuffer*	    m_Framebuffer;
	UniformBufferObject*    m_UBO;

	std::vector<VulkanDescriptor*> m_Descriptors;
	VulkanDescriptorSet*	m_DescriptorSet;
	DescriptorSetManager*	m_DescriptorSetManager;

	bool m_DirtyPipeline;
	bool m_DirtyRenderPass;
	bool m_DirtyFramebuffer;
	bool m_DirtyDescriptorSet;
	bool m_DirtyUBO;

    VulkanImageView* m_RenderTarget0 = nullptr;
    VulkanImageView* m_RenderTarget1 = nullptr;
    VulkanImageView* m_RenderTarget2 = nullptr;
	VulkanImageView* m_RenderTarget3 = nullptr;
	VulkanImageView* m_RenderTarget4 = nullptr;

	std::vector<VulkanImageView*> m_RenderTargets;

    VulkanImageView* m_DepthStencil = nullptr;

	PipelineType m_CurrentPipelinetype;
};

