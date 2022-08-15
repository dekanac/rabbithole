#pragma once

#include <vector>
#include <unordered_map>

class Renderer;
class VulkanTexture;
class VulkanBuffer;

class RabbitPass
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

class RabbitPassManager
{
	SingletonClass(RabbitPassManager)

public:
	void AddPass(RabbitPass* pass) { m_RabbitPasses[pass->GetName()] = pass; }

private:
	std::unordered_map<const char*, RabbitPass*> m_RabbitPasses;

};

#define DECLARE_RABBITPASS(name) \
class name : public RabbitPass \
{ \
public: \
	virtual void DeclareResources(Renderer* renderer) override; \
	virtual void Setup(Renderer* renderer) override; \
	virtual void Render(Renderer* renderer) override; \
	virtual const char* GetName() override { return m_PassName; } \
private: \
	const char* m_PassName = #name; \
};

DECLARE_RABBITPASS(GBufferPass)
DECLARE_RABBITPASS(LightingPass)
DECLARE_RABBITPASS(OutlineEntityPass)
DECLARE_RABBITPASS(CopyToSwapchainPass)
DECLARE_RABBITPASS(SkyboxPass)
DECLARE_RABBITPASS(SSAOPass); 
DECLARE_RABBITPASS(SSAOBlurPass); 
DECLARE_RABBITPASS(BoundingBoxPass); 
DECLARE_RABBITPASS(RTShadowsPass);
DECLARE_RABBITPASS(VolumetricPass);
DECLARE_RABBITPASS(Create3DNoiseTexturePass);
DECLARE_RABBITPASS(ComputeScatteringPass);
DECLARE_RABBITPASS(ApplyVolumetricFogPass);
DECLARE_RABBITPASS(FSR2Pass);
DECLARE_RABBITPASS(TonemappingPass);
DECLARE_RABBITPASS(TextureDebugPass); 
DECLARE_RABBITPASS(ShadowDenoisePrePass); 
DECLARE_RABBITPASS(ShadowDenoiseTileClassificationPass); 
DECLARE_RABBITPASS(ShadowDenoiseFilterPass0); 
DECLARE_RABBITPASS(ShadowDenoiseFilterPass1); 
DECLARE_RABBITPASS(ShadowDenoiseFilterPass2);