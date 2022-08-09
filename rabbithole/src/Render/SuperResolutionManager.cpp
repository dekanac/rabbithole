#include "common.h"
#include "vulkan/precomp.h"`
#include "SuperResolutionManager.h"

#include "Render/Window.h"
#include "Render/Renderer.h"
#include "Render/Camera.h"

#include <fsr2.0/ffx_fsr2.h>
#include <fsr2.0/vk/ffx_fsr2_vk.h>
#include <vulkan/vulkan.h>

void SuperResolutionManager::Init(VulkanDevice* device)
{
	m_UpscaledResolutionWidth = Window::instance().GetExtent().width;
	m_UpscaledResolutionHeight = Window::instance().GetExtent().height;

	m_UpscaleFactor = 4.f/5.f;
	m_Sharpness = 0.666666f;

	m_NativeResolutionWidth = m_UpscaledResolutionWidth * m_UpscaleFactor;
	m_NativeResolutionHeight = m_UpscaledResolutionHeight * m_UpscaleFactor;

	m_Hdr = true;

	CreateFsrContext(device);
}

void SuperResolutionManager::CreateFsrContext(VulkanDevice* device)
{
	// Setup VK interface.

	auto physicalDevice = device->GetPhysicalDevice();
	const size_t scratchBufferSize = ffxFsr2GetScratchMemorySizeVK(physicalDevice);
	void* scratchBuffer = malloc(scratchBufferSize);
	FfxErrorCode errorCode = ffxFsr2GetInterfaceVK(&m_FsrContextDescription.callbacks, scratchBuffer, scratchBufferSize, physicalDevice, vkGetDeviceProcAddr);
	FFX_ASSERT(errorCode == FFX_OK);

	m_FsrContextDescription.device = ffxGetDeviceVK(device->GetGraphicDevice());
	m_FsrContextDescription.maxRenderSize.width = m_NativeResolutionWidth;
	m_FsrContextDescription.maxRenderSize.height = m_NativeResolutionHeight;
	m_FsrContextDescription.displaySize.width = m_UpscaledResolutionWidth;
	m_FsrContextDescription.displaySize.height = m_UpscaledResolutionHeight;
	m_FsrContextDescription.flags = FFX_FSR2_ENABLE_AUTO_EXPOSURE;

	if (m_Hdr) 
	{
		m_FsrContextDescription.flags |= FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE;
	}

	ffxFsr2ContextCreate(&m_FsrContext, &m_FsrContextDescription);
}

void SuperResolutionManager::DestroyFsrContext()
{
	ffxFsr2ContextDestroy(&m_FsrContext);
}

void SuperResolutionManager::PreDraw(UIState* pState)
{
	m_UseTaa = pState->useTaa;

	if (m_UseTaa)
	{
		m_Index++;

		// handle reset and jitter
		const int32_t jitterPhaseCount = ffxFsr2GetJitterPhaseCount(m_NativeResolutionWidth, m_UpscaledResolutionWidth);
		ffxFsr2GetJitterOffset(&m_JitterX, &m_JitterY, m_Index, jitterPhaseCount);

		pState->camera->SetProjectionJitter(2.0f * m_JitterX / (float)pState->renderWidth, -2.0f * m_JitterY / (float)pState->renderHeight);
	}
	else
	{
		pState->camera->SetProjectionJitter(0.f, 0.f);
	}
}

void SuperResolutionManager::Draw(VkCommandBuffer commandBuffer, const FfxUpscaleSetup& cameraSetup, UIState* pState)
{
	FfxFsr2DispatchDescription dispatchParameters = {};
	dispatchParameters.commandList = ffxGetCommandListVK(commandBuffer);
	dispatchParameters.color = ffxGetTextureResourceVK(&m_FsrContext, cameraSetup.unresolvedColorResource->GetResource()->GetImage(), cameraSetup.unresolvedColorResource->GetView()->GetImageView(), cameraSetup.unresolvedColorResource->GetWidth(), cameraSetup.unresolvedColorResource->GetHeight(), GetVkFormatFrom(cameraSetup.unresolvedColorResource->GetFormat()), (wchar_t*)L"FSR2_InputColor");
	dispatchParameters.depth = ffxGetTextureResourceVK(&m_FsrContext, cameraSetup.depthbufferResource->GetResource()->GetImage(), cameraSetup.depthbufferResource->GetView()->GetImageView(), cameraSetup.depthbufferResource->GetWidth(), cameraSetup.depthbufferResource->GetHeight(), GetVkFormatFrom(cameraSetup.depthbufferResource->GetFormat()), (wchar_t*)L"FSR2_InputDepth");
	dispatchParameters.motionVectors = ffxGetTextureResourceVK(&m_FsrContext, cameraSetup.motionvectorResource->GetResource()->GetImage(), cameraSetup.motionvectorResource->GetView()->GetImageView(), cameraSetup.motionvectorResource->GetWidth(), cameraSetup.motionvectorResource->GetHeight(), GetVkFormatFrom(cameraSetup.motionvectorResource->GetFormat()), (wchar_t*)L"FSR2_InputMotionVectors");
	dispatchParameters.output = ffxGetTextureResourceVK(&m_FsrContext, cameraSetup.resolvedColorResource->GetResource()->GetImage(), cameraSetup.resolvedColorResource->GetView()->GetImageView(), cameraSetup.resolvedColorResource->GetWidth(), cameraSetup.resolvedColorResource->GetHeight(), GetVkFormatFrom(cameraSetup.resolvedColorResource->GetFormat()), (wchar_t*)L"FSR2_OutputUpscaledColor", FFX_RESOURCE_STATE_UNORDERED_ACCESS);
	dispatchParameters.jitterOffset.x = m_JitterX;
	dispatchParameters.jitterOffset.y = m_JitterY;
	dispatchParameters.motionVectorScale.x = pState->renderWidth;
	dispatchParameters.motionVectorScale.y = pState->renderHeight;
	dispatchParameters.reset = pState->reset;
	dispatchParameters.enableSharpening = pState->useRcas;
	dispatchParameters.sharpness = pState->sharpness;
	dispatchParameters.frameTimeDelta = pState->deltaTime;
	dispatchParameters.preExposure = 1.0f;
	dispatchParameters.renderSize.width = pState->renderWidth;
	dispatchParameters.renderSize.height = pState->renderHeight;
	dispatchParameters.cameraFar = pState->camera->GetFarPlane();
	dispatchParameters.cameraNear = pState->camera->GetNearPlane();
	dispatchParameters.cameraFovAngleVertical = pState->camera->GetFieldOfViewVerticalRad();
	pState->reset = false;

	FfxErrorCode errorCode = ffxFsr2ContextDispatch(&m_FsrContext, &dispatchParameters);
	FFX_ASSERT(errorCode == FFX_OK);
}

void SuperResolutionManager::GenerateReactiveMask(VkCommandBuffer pCommandList, const FfxUpscaleSetup& cameraSetup, UIState* pState)
{
	//implement this when transparent object are supported
}