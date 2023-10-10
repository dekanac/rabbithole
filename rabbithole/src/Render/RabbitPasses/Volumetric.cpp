#include "Volumetric.h"

#include "Render/RabbitPasses/GBuffer.h"
#include "Render/RabbitPasses/Lighting.h"
#include "Render/RabbitPasses/Shadows.h"

defineResource(VolumetricPass, MediaDensity, VulkanTexture);
defineResource(VolumetricPass, ParamsGPU, VulkanBuffer)
VolumetricPass::VolumetricFogParams VolumetricPass::ParamsCPU = {};

defineResource(ComputeScatteringPass, LightScattering, VulkanTexture);

defineResource(ApplyVolumetricFogPass, Output, VulkanTexture);

defineResource(VolumetricCloudsPass, Output, VulkanTexture);
VolumetricCloudsPass::VolumetricCloudsParams VolumetricCloudsPass::ParamsCPU = {};
defineResource(VolumetricCloudsPass, ParamsGPU, VulkanBuffer)


void VolumetricPass::DeclareResources()
{
	MediaDensity = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {g_VolumetricTexX, g_VolumetricTexY, g_VolumetricTexZ},
			.flags = {TextureFlags::Read | TextureFlags::TransferSrc | TextureFlags::Storage},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Media Density"},
			.samplerType = {SamplerType::Trilinear}
		});

	ParamsGPU = m_Renderer.GetResourceManager().CreateBuffer(m_Renderer.GetVulkanDevice(), BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(VolumetricFogParams)},
			.name = {"Volumetric Fog Params"}
		});
}

void VolumetricPass::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	stateManager.SetComputeShader(m_Renderer.GetShader("CS_Volumetric_CalculateDensity"));

	SetConstantBuffer(4, VolumetricPass::ParamsGPU);
	SetStorageImageWrite(5, VolumetricPass::MediaDensity);
	SetConstantBuffer(7, LightingPass::LightParamsGPU);
	SetConstantBuffer(8, m_Renderer.GetMainConstBuffer());
	SetCombinedImageSampler(9, VolumetricShadowsPass::VolumetricShadows);

	if (m_Renderer.IsImguiReady())
	{
		ImGui::Begin("Volumetric Fog");

		auto& fogParams = VolumetricPass::ParamsCPU;

		static bool fogEnabled = true;
		ImGui::Checkbox("Enable Fog: ", &fogEnabled);
		fogParams.isEnabled = (uint32_t)fogEnabled;

		ImGui::SliderFloat("Fog Amount: ", &(fogParams.fogAmount), 0.0001f, 0.1f);
		ImGui::SliderFloat("Depth Scale Debug: ", &(fogParams.depthScale_debug), 0.1f, 5.f);
		ImGui::SliderFloat("Fog Start Distance ", &(fogParams.fogStartDistance), 0.0f, 20.f);
		ImGui::SliderFloat("Fog Distance ", &(fogParams.fogDistance), 10.f, 128.f);
		ImGui::SliderFloat("Fog Depth Exponent ", &(fogParams.fogDepthExponent), 0.1f, 10.f);

		fogParams.volumetricTexWidth = g_VolumetricTexX;
		fogParams.volumetricTexHeight = g_VolumetricTexY;
		fogParams.volumetricTexDepth = g_VolumetricTexZ;

		ImGui::End();
	}

	VolumetricPass::ParamsGPU->FillBuffer(&VolumetricPass::ParamsCPU);
}

void VolumetricPass::Render()
{
	int texWidth = VolumetricPass::MediaDensity->GetWidth();
	int texHeight = VolumetricPass::MediaDensity->GetHeight();
	int texDepth = VolumetricPass::MediaDensity->GetDepth();

	int dispatchX = GetCSDispatchCount(texWidth, 8);
	int dispatchY = GetCSDispatchCount(texHeight, 4);
	int dispatchZ = GetCSDispatchCount(texDepth, 8);

	m_Renderer.Dispatch(dispatchX, dispatchY, dispatchZ);
}

void Create3DNoiseTexturePass::DeclareResources()
{
}

void Create3DNoiseTexturePass::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	stateManager.SetComputeShader(m_Renderer.GetShader("CS_3DNoiseLUT"));

	SetStorageImageWrite(1, m_Renderer.noise3DLUT);
}

void Create3DNoiseTexturePass::Render()
{
	auto width = m_Renderer.noise3DLUT->GetWidth();
	auto height = m_Renderer.noise3DLUT->GetHeight();
	auto depth = m_Renderer.noise3DLUT->GetDepth();

	m_Renderer.Dispatch(GetCSDispatchCount(width, 8), GetCSDispatchCount(height, 8), GetCSDispatchCount(depth, 8));
}


void ComputeScatteringPass::DeclareResources()
{
	LightScattering = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {g_VolumetricTexX, g_VolumetricTexY, g_VolumetricTexZ},
			.flags = {TextureFlags::Read | TextureFlags::TransferSrc | TextureFlags::Storage},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Scattering Calculation"},
			.samplerType = {SamplerType::Trilinear}
		});
}


void ComputeScatteringPass::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	stateManager.SetComputeShader(m_Renderer.GetShader("CS_ComputeScattering"));

	SetStorageImageRead(0, VolumetricPass::MediaDensity);
	SetStorageImageWrite(1, ComputeScatteringPass::LightScattering);
	SetConstantBuffer(2, VolumetricPass::ParamsGPU);
}


void ComputeScatteringPass::Render()
{
	int texWidth = ComputeScatteringPass::LightScattering->GetWidth();
	int texHeight = ComputeScatteringPass::LightScattering->GetHeight();

	static const int threadGroupWorkRegionDim = 8;
	int dispatchX = (texWidth + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
	int dispatchY = (texHeight + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;

	m_Renderer.Dispatch(dispatchX, dispatchY, 1);
}

void ApplyVolumetricFogPass::DeclareResources()
{
	Output = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::TransferSrc},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Volumetric Fog Output"},
		});
}

void ApplyVolumetricFogPass::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	stateManager.SetVertexShader(m_Renderer.GetShader("VS_PassThrough"));
	stateManager.SetPixelShader(m_Renderer.GetShader("FS_ApplyVolumetricFog"));

	SetCombinedImageSampler(0, LightingPass::MainLighting);
	SetCombinedImageSampler(1, CopyDepthPass::DepthR32);
	SetCombinedImageSampler(2, ComputeScatteringPass::LightScattering);
	SetConstantBuffer(3, m_Renderer.GetMainConstBuffer());
	SetConstantBuffer(4, VolumetricPass::ParamsGPU);

	SetRenderTarget(0, ApplyVolumetricFogPass::Output);
}

void ApplyVolumetricFogPass::Render()
{
	m_Renderer.DrawFullScreenQuad();
}

void VolumetricCloudsPass::DeclareResources() 
{
	Output = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::TransferSrc},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Volumetric Clouds Output"},
		});

	ParamsGPU = m_Renderer.GetResourceManager().CreateBuffer(m_Renderer.GetVulkanDevice(), BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(VolumetricCloudsParams)},
			.name = {"Volumetric Clouds Params"}
		});
}

void VolumetricCloudsPass::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	stateManager.SetVertexShader(m_Renderer.GetShader("VS_PassThrough"));
	stateManager.SetPixelShader(m_Renderer.GetShader("FS_VolumetricClouds"));

	m_Renderer.BindViewport(0, 0, static_cast<float>(GetNativeWidth), static_cast<float>(GetNativeHeight));
	stateManager.SetRenderPassExtent({ GetNativeWidth , GetNativeHeight });

	auto pipelineInfo = stateManager.GetPipelineInfo();
	pipelineInfo->SetColorWriteMask(0, ColorWriteMaskFlags::RGBA);
	pipelineInfo->SetDepthTestEnabled(true);

	//alpha blending
	pipelineInfo->SetAlphaBlendEnabled(0, true);
	pipelineInfo->SetAlphaBlendFunction(0, BlendValue::SrcAlpha, BlendValue::InvSrcAlpha);
	pipelineInfo->SetAlphaBlendOperation(0, BlendOperation::Add);

	auto renderPassInfo = stateManager.GetRenderPassInfo();
	renderPassInfo->InitialRenderTargetState = ResourceState::RenderTarget;
	renderPassInfo->FinalRenderTargetState = ResourceState::RenderTarget;

	SetConstantBuffer(0, m_Renderer.GetMainConstBuffer());
	SetCombinedImageSampler(1, m_Renderer.noise3DLUT);
	SetCombinedImageSampler(2, CopyDepthPass::DepthR32);
	SetConstantBuffer(3, ParamsGPU);
	SetConstantBuffer(4, LightingPass::LightParamsGPU);

	SetRenderTarget(0, LightingPass::MainLighting);
	SetDepthStencil(GBufferPass::Depth);

	if (m_Renderer.IsImguiReady())
	{
		ImGui::Begin("Volumetric Clouds");

		auto& fogParams = VolumetricCloudsPass::ParamsCPU;

		ImGui::SliderFloat("SunPhaseValue: ", &(fogParams.SunPhaseValue), 0.1f, 2.0f);
		ImGui::SliderFloat("VolumeLightAbsorption: ", &(fogParams.VolumeLightAbsorption), 0.1f, 1.f);
		ImGui::SliderFloat("ApproxFadeDistance: ", &(fogParams.ApproxFadeDistance), 0.0f, 150.f);
		ImGui::SliderFloat("CurveAggressivness: ", &(fogParams.CurveAggressivness), 1.f, 100.f);
		ImGui::SliderFloat("EdgeGradient: ", &(fogParams.EdgeGradient), 0.01f, 8.f);
		ImGui::SliderFloat("LightTransmittance: ", &(fogParams.LightMarchSteps), 1.f, 10.f);
		ImGui::SliderFloat("SomeParam1: ", &(fogParams.SomeParam1), 0.1f, 10.f);
		ImGui::SliderFloat("SomeParam2: ", &(fogParams.SomeParam2), 0.1f, 10.f);
		ImGui::SliderFloat("SomeParam3: ", &(fogParams.SomeParam3), 0.1f, 10.f);
		ImGui::SliderFloat("SomeParam4: ", &(fogParams.SomeParam4), 0.1f, 10.f);
		ImGui::SliderFloat("SomeParam5: ", &(fogParams.SomeParam5), 0.1f, 10.f);
		ImGui::SliderFloat("SomeParam6: ", &(fogParams.SomeParam6), 0.1f, 10.f);

		ImGui::End();
	}

	VolumetricCloudsPass::ParamsGPU->FillBuffer(&VolumetricCloudsPass::ParamsCPU);

}

void VolumetricCloudsPass::Render() 
{
	m_Renderer.DrawFullScreenQuad();
}
