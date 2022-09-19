#pragma once

#include <vector>
#include <unordered_map>

#include "Render/Renderer.h"

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
	void SchedulePasses();
	void DeclareResources(Renderer* renderer);
	void ExecutePasses(Renderer* renderer);
	void ExecuteOneTimePasses(Renderer* renderer);
	void Destroy();

public:
	void AddPass(RabbitPass* pass, bool executeOnce = false) { 
		m_RabbitPasses[pass->GetName()] = pass; 
		executeOnce ? m_RabbitPassesOneTimeExecute.push_back(pass) : m_RabbitPassesToExecute.push_back(pass);
	}

private:
	std::unordered_map<const char*, RabbitPass*> m_RabbitPasses;
	std::list<RabbitPass*> m_RabbitPassesToExecute;
	std::list<RabbitPass*> m_RabbitPassesOneTimeExecute;
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

#define declareResource(name, type) static type* name
#define declareResourceArray(name, type, slices) static type* name[slices]
#define defineResource(pass, name, type) type* pass::name = nullptr;
#define defineResourceArray(pass, name, type, slices) type* pass::name[slices] = { nullptr };

//########################################
BEGIN_DECLARE_RABBITPASS(GBufferPass)
	declareResource(Albedo, VulkanTexture);
	declareResource(Normals, VulkanTexture);
	declareResource(Velocity, VulkanTexture);
	declareResource(WorldPosition, VulkanTexture);
	declareResource(Depth, VulkanTexture);
END_DECLARE_RABBITPASS
//########################################

//########################################
BEGIN_DECLARE_RABBITPASS(LightingPass)
	
	declareResource(MainLighting, VulkanTexture);
	declareResource(LightParamsGPU, VulkanBuffer);

END_DECLARE_RABBITPASS
//########################################

//########################################
BEGIN_DECLARE_RABBITPASS(OutlineEntityPass)
END_DECLARE_RABBITPASS
//########################################

//########################################
BEGIN_DECLARE_RABBITPASS(CopyToSwapchainPass)
END_DECLARE_RABBITPASS
//########################################

//########################################
BEGIN_DECLARE_RABBITPASS(SkyboxPass)
	declareResource(Main, VulkanTexture);
END_DECLARE_RABBITPASS
//########################################

//########################################
BEGIN_DECLARE_RABBITPASS(SSAOPass);

	float lerp(float a, float b, float f)
	{
		return a + f * (b - a);
	}

	struct SSAOSamples
	{
		rabbitVec3f samples[64];
	};
	
	struct SSAOParams
	{
		float	radius;
		float	bias;
		float	resWidth;
		float	resHeight;
		float	power;
		int		kernelSize;
		bool	ssaoOn;
	};

	declareResource(Output, VulkanTexture);
	declareResource(Noise, VulkanTexture);
	declareResource(Samples, VulkanBuffer);
	declareResource(ParamsGPU, VulkanBuffer);
	static SSAOParams ParamsCPU;

END_DECLARE_RABBITPASS
//########################################

//########################################
BEGIN_DECLARE_RABBITPASS(SSAOBlurPass);
	declareResource(BluredOutput, VulkanTexture);
END_DECLARE_RABBITPASS
//########################################
// 
//########################################
BEGIN_DECLARE_RABBITPASS(RTShadowsPass);
	declareResource(ShadowMask, VulkanTexture);
	static uint32_t ShadowResX;
	static uint32_t ShadowResY;
END_DECLARE_RABBITPASS
//########################################

//########################################
BEGIN_DECLARE_RABBITPASS(ShadowDenoisePrePass);

	struct DenoiseBufferDimensions
	{
		uint32_t dimensions[2];
	};

	void PrepareDenoisePass(Renderer* renderer, uint32_t shadowSlice);
	
	declareResource(BufferDimensions, VulkanBuffer);
	declareResourceArray(ShadowData, VulkanBuffer, MAX_NUM_OF_LIGHTS);
END_DECLARE_RABBITPASS
//########################################

//########################################
BEGIN_DECLARE_RABBITPASS(ShadowDenoiseTileClassificationPass);
	
	struct DenoiseShadowData
	{
		rabbitVec3f		Eye;
		int				FirstFrame;
		int32_t			BufferDimensions[2];
		float			InvBufferDimensions[2];
		rabbitMat4f		ProjectionInverse;
		rabbitMat4f		ReprojectionMatrix;
		rabbitMat4f		ViewProjectionInverse;
	};
	
	uint32_t GetCurrentIDFromFrameIndex(uint32_t id) { return (Renderer::instance().GetCurrentFrameIndex() + id) % 2; }
	void ClassifyTiles(Renderer* renderer, uint32_t shadowSlice);
	
	declareResource(LastFrameDepth, VulkanTexture);
	declareResourceArray(Moments0, VulkanTexture, MAX_NUM_OF_LIGHTS);
	declareResourceArray(Moments1, VulkanTexture, MAX_NUM_OF_LIGHTS);
	declareResourceArray(Reprojection0, VulkanTexture, MAX_NUM_OF_LIGHTS);
	declareResourceArray(Reprojection1, VulkanTexture, MAX_NUM_OF_LIGHTS);

	declareResourceArray(TileMetadata, VulkanBuffer, MAX_NUM_OF_LIGHTS);
	declareResource(ReprojectionInfo, VulkanBuffer);

END_DECLARE_RABBITPASS
//########################################

//########################################
BEGIN_DECLARE_RABBITPASS(ShadowDenoiseFilterPass);

	struct DenoiseShadowFilterData
	{
		rabbitMat4f ProjectionInverse;
		int32_t     BufferDimensions[2];
		float		InvBufferDimensions[2];
		float		DepthSimilaritySigma;
	};

	void RenderFilterPass0(Renderer* renderer, uint32_t shadowSlice);
	void RenderFilterPass1(Renderer* renderer, uint32_t shadowSlice);
	void RenderFilterPass2(Renderer* renderer, uint32_t shadowSlice);

	declareResource(FilterData, VulkanBuffer);
	declareResource(ShadowMask, VulkanTexture);

END_DECLARE_RABBITPASS
//########################################

//########################################
BEGIN_DECLARE_RABBITPASS(VolumetricPass);

	struct VolumetricFogParams
	{
		uint32_t	isEnabled = true;
		float		fogAmount = 0.006f;
		float		depthScale_debug = 2.f;
		float		fogStartDistance = 0.1f;
		float		fogDistance = 64.f;
	};

	declareResource(MediaDensity, VulkanTexture);
	declareResource(ParamsGPU, VulkanBuffer);

	static VolumetricFogParams ParamsCPU;

END_DECLARE_RABBITPASS
//########################################

//########################################
BEGIN_DECLARE_RABBITPASS(ComputeScatteringPass);
	declareResource(LightScattering, VulkanTexture);
END_DECLARE_RABBITPASS
//########################################

//########################################
BEGIN_DECLARE_RABBITPASS(Create3DNoiseTexturePass);
END_DECLARE_RABBITPASS
//########################################
// 
//########################################
BEGIN_DECLARE_RABBITPASS(ApplyVolumetricFogPass);
	declareResource(Output, VulkanTexture);
END_DECLARE_RABBITPASS
//########################################
// 
//########################################
BEGIN_DECLARE_RABBITPASS(FSR2Pass);
	declareResource(Output, VulkanTexture);
END_DECLARE_RABBITPASS
//########################################

//########################################
BEGIN_DECLARE_RABBITPASS(TonemappingPass);
	declareResource(Output, VulkanTexture);
END_DECLARE_RABBITPASS
//########################################

//########################################
BEGIN_DECLARE_RABBITPASS(TextureDebugPass);

	struct DebugTextureParams
	{
		uint32_t hasMips = false;
		int mipSlice = 0;
		int mipCount = 1;
	
		uint32_t isArray = false;
		int arraySlice = 0;
		int arrayCount = 1;
	
		uint32_t showR = true;
		uint32_t showG = true;
		uint32_t showB = true;
		uint32_t showA = true;
	
		bool is3D;
		float texture3DDepthScale = 0.f;
	};

	declareResource(Output, VulkanTexture);
	declareResource(ParamsGPU, VulkanBuffer);

	static DebugTextureParams ParamsCPU;

END_DECLARE_RABBITPASS
//########################################
