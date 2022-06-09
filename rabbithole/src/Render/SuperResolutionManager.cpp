#include "common.h"
#include "vulkan/precomp.h"`
#include "SuperResolutionManager.h"

#include "Window.h"

#define A_CPU
#include "fsr/ffx_a.h"
#include "fsr/ffx_fsr1.h"

void SuperResolutionManager::Init()
{
	m_UpscaledResolutionWidth = Window::instance().GetExtent().width;
	m_UpscaledResolutionHeight = Window::instance().GetExtent().height;

	m_UpscaleFactor = 4.f/5.f;
	m_Sharpness = 0.666666f;

	m_NativeResolutionWidth = m_UpscaledResolutionWidth * m_UpscaleFactor;
	m_NativeResolutionHeight = m_UpscaledResolutionHeight * m_UpscaleFactor;
}

void SuperResolutionManager::CalculateFSRParamsEASU(FSRParams& params)
{
	FsrEasuCon(
		reinterpret_cast<AU1*>(&params.Const0),
		reinterpret_cast<AU1*>(&params.Const1),
		reinterpret_cast<AU1*>(&params.Const2),
		reinterpret_cast<AU1*>(&params.Const3),
		static_cast<AF1>(m_NativeResolutionWidth),
		static_cast<AF1>(m_NativeResolutionHeight),
		static_cast<AF1>(m_NativeResolutionWidth),
		static_cast<AF1>(m_NativeResolutionHeight),
		static_cast<AF1>(m_UpscaledResolutionWidth),
		static_cast<AF1>(m_UpscaledResolutionHeight)
	);
}

void SuperResolutionManager::CalculateFSRParamsRCAS(FSRParams& params)
{
	FsrRcasCon(
		reinterpret_cast<AU1*>(&params.Const0Rcas),
		static_cast<AF1>(m_Sharpness)
	);
}