#pragma once

#include <utility>

#include <fsr2.0/ffx_fsr2.h>

class VulkanTexture;
struct UIState;

class SuperResolutionManager
{
	SingletonClass(SuperResolutionManager)

public:

	typedef struct
	{
		rabbitMat4f   cameraView;
		rabbitMat4f   cameraProj;
		rabbitMat4f   cameraViewInv;
		rabbitVec4f   cameraPos;
	} FfxCameraSetup;

	typedef struct
	{
		FfxCameraSetup  cameraSetup;

		VulkanTexture* unresolvedColorResource;               // input0
		VulkanTexture* motionvectorResource;                  // input1
		VulkanTexture* depthbufferResource;                   // input2
		VulkanTexture* reactiveMapResource;                   // input3
		VulkanTexture* transparencyAndCompositionResource;    // input4
		VulkanTexture* opaqueOnlyColorResource;               // input5
		VulkanTexture* resolvedColorResource;                 // output

	} FfxUpscaleSetup;

	void Init(VulkanDevice* device);

	const std::pair<uint32_t, uint32_t> GetNativeResolution() { return std::make_pair(m_NativeResolutionWidth, m_NativeResolutionHeight); }
	const std::pair<uint32_t, uint32_t> GetUpscaledResolution() { return std::make_pair(m_UpscaledResolutionWidth, m_UpscaledResolutionHeight); }

	const float GetUpscaleFactor() { return m_UpscaleFactor; }

	void CreateFsrContext(VulkanDevice* device);
	void DestroyFsrContext();

	void PreDraw(UIState* pState);
	void Draw(VkCommandBuffer commandBuffer, const FfxUpscaleSetup& cameraSetup, UIState* pState);
	void GenerateReactiveMask(VkCommandBuffer pCommandList, const FfxUpscaleSetup& cameraSetup, UIState* pState);

private:
	uint32_t m_NativeResolutionWidth;
	uint32_t m_NativeResolutionHeight;
	uint32_t m_UpscaledResolutionWidth;
	uint32_t m_UpscaledResolutionHeight;

	float m_UpscaleFactor;
	float m_Sharpness;

	float m_JitterX;
	float m_JitterY;
	bool m_UseTaa;
	bool m_Hdr;
	uint32_t m_Index;

	FfxFsr2ContextDescription	m_FsrContextDescription = {};
	FfxFsr2Context				m_FsrContext;
};


#define GetNativeWidth (SuperResolutionManager::instance().GetNativeResolution().first)
#define GetNativeHeight (SuperResolutionManager::instance().GetNativeResolution().second)

#define GetUpscaledWidth (SuperResolutionManager::instance().GetUpscaledResolution().first)
#define GetUpscaledHeight (SuperResolutionManager::instance().GetUpscaledResolution().second)