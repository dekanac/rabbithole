#pragma once

#include <vector>

class Renderer;
class VulkanTexture;
class VulkanBuffer;

class RenderPass
{
public:
	virtual void DeclareResources(Renderer* renderer) = 0;
	virtual void Setup(Renderer* renderer) = 0;
	virtual void Render(Renderer* renderer) = 0;
	virtual const char* GetName() = 0;

protected:
	void SetCombinedImageSampler(Renderer* renderer, int slot, VulkanTexture* texture);
	void SetSampledImage(Renderer* renderer, int slot, VulkanTexture* texture);
	void SetStorageImage(Renderer* renderer, int slot, VulkanTexture* texture);

    void SetConstantBuffer(Renderer* renderer, int slot, VulkanBuffer* buffer);
    void SetStorageBuffer(Renderer* renderer, int slot, VulkanBuffer* buffer);
	void SetSampler(Renderer* renderer, int slot, VulkanTexture* texture);

	void SetRenderTarget(Renderer* renderer, int slot, VulkanTexture* texture);
	void SetDepthStencil(Renderer* renderer, VulkanTexture* texture);
};

#define DECLARE_RENDERPASS(name) \
class name : public RenderPass \
{ \
public: \
	virtual void DeclareResources(Renderer* renderer); \
	virtual void Setup(Renderer* renderer); \
	virtual void Render(Renderer* renderer); \
	virtual const char* GetName() { return m_PassName; } \
private: \
	const char* m_PassName = #name; \
};

DECLARE_RENDERPASS(GBufferPass)
DECLARE_RENDERPASS(LightingPass)
DECLARE_RENDERPASS(OutlineEntityPass)
DECLARE_RENDERPASS(CopyToSwapchainPass)
DECLARE_RENDERPASS(SkyboxPass)
DECLARE_RENDERPASS(SSAOPass); 
DECLARE_RENDERPASS(SSAOBlurPass); 
DECLARE_RENDERPASS(BoundingBoxPass); 
DECLARE_RENDERPASS(RTShadowsPass);
DECLARE_RENDERPASS(FSREASUPass);
DECLARE_RENDERPASS(FSRRCASPass);
DECLARE_RENDERPASS(TAAPass); 
DECLARE_RENDERPASS(TAASharpenerPass);