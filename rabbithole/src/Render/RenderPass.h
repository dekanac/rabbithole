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
	virtual void DeclareResources(Renderer* renderer) override; \
	virtual void Setup(Renderer* renderer) override; \
	virtual void Render(Renderer* renderer) override; \
	virtual const char* GetName() override { return m_PassName; } \
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
DECLARE_RENDERPASS(VolumetricPass);
DECLARE_RENDERPASS(Create3DNoiseTexturePass);
DECLARE_RENDERPASS(ComputeScatteringPass);
DECLARE_RENDERPASS(ApplyVolumetricFogPass);
DECLARE_RENDERPASS(FSR2Pass);
DECLARE_RENDERPASS(TonemappingPass);
DECLARE_RENDERPASS(TextureDebugPass); 
DECLARE_RENDERPASS(ShadowDenoisePrePass); 
DECLARE_RENDERPASS(ShadowDenoiseTileClassificationPass); 
DECLARE_RENDERPASS(ShadowDenoiseFilterPass0); 
DECLARE_RENDERPASS(ShadowDenoiseFilterPass1); 
DECLARE_RENDERPASS(ShadowDenoiseFilterPass2);