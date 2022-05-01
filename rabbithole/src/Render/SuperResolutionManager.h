#pragma once

#include <utility>

class VulkanTexture;

struct FSRParams
{
	uint32_t Const0[4];
	uint32_t Const1[4];
	uint32_t Const2[4];
	uint32_t Const3[4];
	uint32_t Sample[4];
	uint32_t Const0Rcas[4];
};

class SuperResolutionManager
{
	SingletonClass(SuperResolutionManager)

public:
	void Init();

	const std::pair<uint32_t, uint32_t> GetNativeResolution() { return std::make_pair(m_NativeResolutionWidth, m_NativeResolutionHeight); }
	const std::pair<uint32_t, uint32_t> GetUpscaledResolution() { return std::make_pair(m_UpscaledResolutionWidth, m_UpscaledResolutionHeight); }

	const float GetUpscaleFactor() { return m_UpscaleFactor; }

	void CalculateFSRParamsEASU(FSRParams& params);
	void CalculateFSRParamsRCAS(FSRParams& params);

private:
	uint32_t m_NativeResolutionWidth;
	uint32_t m_NativeResolutionHeight;

	float m_UpscaleFactor;
	float m_Sharpness;

	uint32_t m_UpscaledResolutionWidth;
	uint32_t m_UpscaledResolutionHeight;
};


#define GetNativeWidth (SuperResolutionManager::instance().GetNativeResolution().first)
#define GetNativeHeight (SuperResolutionManager::instance().GetNativeResolution().second)

#define GetUpscaledWidth (SuperResolutionManager::instance().GetUpscaledResolution().first)
#define GetUpscaledHeight (SuperResolutionManager::instance().GetUpscaledResolution().second)