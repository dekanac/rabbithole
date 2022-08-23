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
	SingletonClass(RabbitPassManager);

public:
	void AddPass(RabbitPass* pass) { m_RabbitPasses[pass->GetName()] = pass; }

private:
	std::unordered_map<const char*, RabbitPass*> m_RabbitPasses;

};

#define BEGIN_DECLARE_RABBITPASS(name) \
class name : public RabbitPass \
{ \
public: \
	virtual void DeclareResources(Renderer* renderer) override; \
	virtual void Setup(Renderer* renderer) override; \
	virtual void Render(Renderer* renderer) override; \
	virtual const char* GetName() override { return m_PassName; } \
private: \
	const char* m_PassName = #name; \
public:

#define END_DECLARE_RABBITPASS };

BEGIN_DECLARE_RABBITPASS(GBufferPass)										
END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(LightingPass)
END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(OutlineEntityPass)
END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(CopyToSwapchainPass)
END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(SkyboxPass)
END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(SSAOPass);
END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(SSAOBlurPass);
END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(BoundingBoxPass);
END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(RTShadowsPass);
END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(VolumetricPass);
END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(Create3DNoiseTexturePass);
END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(ComputeScatteringPass);
END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(ApplyVolumetricFogPass);
END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(FSR2Pass);
END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(TonemappingPass);
END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(TextureDebugPass);
END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(ShadowDenoisePrePass);
void PrepareDenoisePass(Renderer* renderer, uint32_t shadowSlice);
END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(ShadowDenoiseTileClassificationPass);
void ClassifyTiles(Renderer* renderer, uint32_t shadowSlice);
END_DECLARE_RABBITPASS

BEGIN_DECLARE_RABBITPASS(ShadowDenoiseFilterPass);
void RenderFilterPass0(Renderer* renderer, uint32_t shadowSlice);
void RenderFilterPass1(Renderer* renderer, uint32_t shadowSlice);
void RenderFilterPass2(Renderer* renderer, uint32_t shadowSlice);
END_DECLARE_RABBITPASS
