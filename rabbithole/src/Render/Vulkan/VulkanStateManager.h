#pragma once

class VulkanPipeline;
class VulkanRenderPass;
class VulkanFramebuffer;
struct UniformBufferObject;

typedef VkExtent2D Extent2D;

struct PushConstant
{
	uint32_t data[32];
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
	PipelineConfigInfo* GetPipelineInfo() { return m_PipelineConfig; }
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
	VulkanRenderPass*	    GetRenderPass() const { return m_RenderPass; }
    RenderPassConfigInfo*   GetRenderPassInfo() { return m_RenderPassConfig; }
	void SetRenderPass(VulkanRenderPass* renderPass) { m_RenderPass = renderPass; m_DirtyRenderPass = false; }
	bool GetRenderPassDirty() { return m_DirtyRenderPass; }
	void SetRenderPassDirty(bool dirty) { m_DirtyRenderPass = dirty; }

	//framebuffer
	VulkanFramebuffer*	GetFramebuffer() const { return m_Framebuffer; }
	void SetFramebuffer(VulkanFramebuffer* framebuffer) { m_Framebuffer = framebuffer; m_DirtyFramebuffer = false; }
	bool GetFramebufferDirty() { return m_DirtyFramebuffer; }
	void SetFramebufferDirty(bool dirty) { m_DirtyFramebuffer = dirty; }

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

    std::vector<VulkanImageView*>&  GetRenderTargets();
    VulkanImageView*                GetDepthStencil() const { return m_DepthStencil; }

	void SetPushConst(PushConstant& pc) { m_PushConst = pc; m_ShouldBindPushConst = true; }
	PushConstant& GetPushConst() { return m_PushConst; }
	bool ShouldBindPushConst() const { return m_ShouldBindPushConst; }
	void SetShouldBindPushConst(bool should) { m_ShouldBindPushConst = should; }

    void SetRenderTarget0(VulkanImageView* rt);
    void SetRenderTarget1(VulkanImageView* rt);
    void SetRenderTarget2(VulkanImageView* rt);
    void SetRenderTarget3(VulkanImageView* rt);
	void SetRenderTarget4(VulkanImageView* rt);
	void SetDepthStencil(VulkanImageView* ds);

	void ShouldCleanColor(bool clean);
	void ShouldCleanDepth(bool clean);

	void SetCombinedImageSampler(uint32_t slot, VulkanTexture* texture, uint32_t mipSlice = 0);
	void SetConstantBuffer(uint32_t slot, VulkanBuffer* buffer);
	void SetStorageImage(uint32_t slot, VulkanImageView* view);
	void SetStorageBuffer(uint32_t slot, VulkanBuffer* buffer);
	void SetSampledImage(uint32_t slot, VulkanImageView* view);
	void SetSampler(uint32_t slot, VulkanImageSampler* sampler);

	VulkanDescriptorSet* FinalizeDescriptorSet(VulkanDevice& device, const VulkanDescriptorPool* pool);
	uint8_t GetRenderTargetCount();
	bool HasDepthStencil() { return m_DepthStencil ? true : false; }

	Extent2D GetFramebufferExtent() const { return m_FramebufferExtent; }
	void SetFramebufferExtent(Extent2D extent) { m_FramebufferExtent = extent; m_DirtyFramebuffer = true; }

	PipelineType GetCurrentPipelineType() { return m_CurrentPipelinetype; }

	void Reset();
	void UpdateResourceStage(ManagableResource* texture);

private:
	PipelineConfigInfo*     m_PipelineConfig;
	VulkanPipeline*		    m_Pipeline;
    RenderPassConfigInfo*   m_RenderPassConfig;
	VulkanRenderPass*	    m_RenderPass;
	VulkanFramebuffer*	    m_Framebuffer;
	UniformBufferObject*    m_UBO;

	std::vector<VulkanDescriptor*>	m_Descriptors;
	VulkanDescriptorSet*			m_DescriptorSet;

	bool m_DirtyPipeline = true;
	bool m_DirtyRenderPass = true;
	bool m_DirtyFramebuffer = true;
	bool m_DirtyDescriptorSet = true;
	bool m_DirtyUBO = true;

    VulkanImageView* m_RenderTarget0 = nullptr;
    VulkanImageView* m_RenderTarget1 = nullptr;
    VulkanImageView* m_RenderTarget2 = nullptr;
	VulkanImageView* m_RenderTarget3 = nullptr;
	VulkanImageView* m_RenderTarget4 = nullptr;

	std::vector<VulkanImageView*>	m_RenderTargets;
    VulkanImageView*				m_DepthStencil = nullptr;

	Extent2D						m_FramebufferExtent{};

	PushConstant					m_PushConst;
	bool							m_ShouldBindPushConst = false;

	PipelineType m_CurrentPipelinetype;
};

