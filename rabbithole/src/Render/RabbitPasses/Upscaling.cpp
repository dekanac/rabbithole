#include "Upscaling.h"

#include "Render/RabbitPasses/GBuffer.h"
#include "Render/RabbitPasses/Volumetric.h"

defineResource(FSR2Pass, Output, VulkanTexture);

void FSR2Pass::DeclareResources()
{
	Output = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetUpscaledWidth, GetUpscaledHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::Storage},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"FSR2 Output"},
			.arraySize = 1,
			.isCube = false,
			.multisampleType = MultisampleType::Sample_1,
			.samplerType = SamplerType::Bilinear,
			.addressMode = AddressMode::Clamp
		});
}

void FSR2Pass::Setup()
{
	m_Renderer.ResourceBarrier(FSR2Pass::Output, ResourceState::GenericRead, ResourceState::GeneralComputeReadWrite, ResourceStage::Graphics, ResourceStage::Compute);
	m_Renderer.ResourceBarrier(ApplyVolumetricFogPass::Output, ResourceState::RenderTarget, ResourceState::GenericRead, ResourceStage::Graphics, ResourceStage::Compute);
}

void FSR2Pass::Render()
{
	SuperResolutionManager::FfxUpscaleSetup fsrSetup{};

	const auto& cameraState = m_Renderer.GetCameraState();

	fsrSetup.cameraSetup.cameraPos = rabbitVec4f{ cameraState.CameraPosition, 1.0f };
	fsrSetup.cameraSetup.cameraProj = cameraState.ProjectionMatrix;
	fsrSetup.cameraSetup.cameraView = cameraState.ViewMatrix;
	fsrSetup.cameraSetup.cameraViewInv = cameraState.ViewInverseMatrix;

	fsrSetup.depthbufferResource = CopyDepthPass::DepthR32;
	fsrSetup.motionvectorResource = GBufferPass::Velocity;
	fsrSetup.unresolvedColorResource = ApplyVolumetricFogPass::Output;
	fsrSetup.resolvedColorResource = FSR2Pass::Output;

	SuperResolutionManager::instance().Draw(m_Renderer.GetCurrentCommandBuffer(), fsrSetup, &m_Renderer.GetUIState());

	m_Renderer.ResourceBarrier(FSR2Pass::Output, ResourceState::GeneralComputeReadWrite, ResourceState::GenericRead, ResourceStage::Compute, ResourceStage::Compute);
}