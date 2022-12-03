#include "Volumetric.h"

#include "Render/RabbitPasses/GBuffer.h"
#include "Render/RabbitPasses/Lighting.h"

defineResource(VolumetricPass, MediaDensity, VulkanTexture);
defineResource(VolumetricPass, ParamsGPU, VulkanBuffer)
VolumetricPass::VolumetricFogParams VolumetricPass::ParamsCPU = {};

defineResource(ComputeScatteringPass, LightScattering, VulkanTexture);

defineResource(ApplyVolumetricFogPass, Output, VulkanTexture);

void VolumetricPass::DeclareResources()
{
	MediaDensity = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {160, 90, 128},
			.flags = {TextureFlags::Read | TextureFlags::TransferSrc | TextureFlags::Storage},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Media Density"},
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

	stateManager.SetComputeShader(m_Renderer.GetShader("CS_Volumetric"));

	SetStorageBufferRead(0, m_Renderer.vertexBuffer);
	SetStorageBufferRead(1, m_Renderer.trianglesBuffer);
	SetStorageBufferRead(2, m_Renderer.triangleIndxsBuffer);
	SetStorageBufferRead(3, m_Renderer.cfbvhNodesBuffer);
	SetConstantBuffer(4, VolumetricPass::ParamsGPU);
	SetStorageImageWrite(5, VolumetricPass::MediaDensity);
	SetCombinedImageSampler(6, m_Renderer.noise3DLUT);
	SetConstantBuffer(7, LightingPass::LightParamsGPU);
	SetConstantBuffer(8, m_Renderer.GetMainConstBuffer());

	if (m_Renderer.IsImguiReady())
	{
		ImGui::Begin("Volumetric Fog:");

		auto& fogParams = VolumetricPass::ParamsCPU;

		static bool fogEnabled;
		ImGui::Checkbox("Enable Fog: ", &fogEnabled);
		fogParams.isEnabled = (uint32_t)fogEnabled;

		ImGui::SliderFloat("Fog Amount: ", &(fogParams.fogAmount), 0.0001f, 0.1f);
		ImGui::SliderFloat("Depth Scale Debug: ", &(fogParams.depthScale_debug), 0.1f, 5.f);
		ImGui::SliderFloat("Fog Start Distance ", &(fogParams.fogStartDistance), 0.01f, 20.f);
		ImGui::SliderFloat("Fog Distance ", &(fogParams.fogDistance), 10.f, 256.f);

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

	SetCombinedImageSampler(0, m_Renderer.noise2DTexture);
	SetStorageImageWrite(1, m_Renderer.noise3DLUT);
}

void Create3DNoiseTexturePass::Render()
{
	m_Renderer.Dispatch(256, 256, 256);
}


void ComputeScatteringPass::DeclareResources()
{
	LightScattering = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {160, 90, 64},
			.flags = {TextureFlags::Read | TextureFlags::TransferSrc | TextureFlags::Storage},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Scattering Calculation"},
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
