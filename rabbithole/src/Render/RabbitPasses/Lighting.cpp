#include "Lighting.h"

#include "Render/RabbitPasses/GBuffer.h"
#include "Render/RabbitPasses/AmbientOcclusion.h"
#include "Render/RabbitPasses/Shadows.h"

defineResource(LightingPass, MainLighting, VulkanTexture);
defineResource(LightingPass, LightParamsGPU, VulkanBuffer);

void LightingPass::DeclareResources()
{
	MainLighting = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::TransferSrc},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Lighting Main"},
		});

	LightParamsGPU = m_Renderer.GetResourceManager().CreateBuffer(m_Renderer.GetVulkanDevice(), BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(LightParams) * MAX_NUM_OF_LIGHTS},
			.name = {"Light params"}
		});
}

void LightingPass::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	stateManager.SetVertexShader(m_Renderer.GetShader("VS_PassThrough"));
	stateManager.SetPixelShader(m_Renderer.GetShader("FS_PBR"));

	//fill the light buffer
	if (m_Renderer.imguiReady)
	{
		ImGui::Begin("Light params");

		auto& lightParams = m_Renderer.lights;

		ImGui::Text("Sun");
		ImGui::ColorEdit3("col0: ", lightParams[0].color);
		ImGui::SliderFloat("intensity0: ", &(lightParams[0]).intensity, 0.f, 50.f);
		ImGui::SliderFloat3("position0: ", lightParams[0].position, -200.f, 200.f);
		ImGui::SliderFloat("size0: ", &lightParams[0].size, 0.1f, 10.f);

		ImGui::Text("Light 1");
		ImGui::ColorEdit3("col", lightParams[1].color);
		ImGui::SliderFloat("radius1: ", &(lightParams[1]).radius, 0.f, 50.f);
		ImGui::SliderFloat("intensity1: ", &(lightParams[1]).intensity, 0.f, 50.f);
		ImGui::SliderFloat3("position1: ", lightParams[1].position, -20.f, 20.f);
		ImGui::SliderFloat("size1: ", &lightParams[1].size, 0.1f, 10.f);

		ImGui::Text("Light 2");
		ImGui::ColorEdit3("col2", lightParams[2].color);
		ImGui::SliderFloat("radius2: ", &(lightParams[2]).radius, 0.f, 50.f);
		ImGui::SliderFloat("intensity2: ", &(lightParams[2]).intensity, 0.f, 50.f);
		ImGui::SliderFloat3("position2: ", lightParams[2].position, -20.f, 20.f);
		ImGui::SliderFloat("size:2 ", &lightParams[2].size, 0.1f, 10.f);

		ImGui::Text("Light 3");
		ImGui::ColorEdit3("col3", lightParams[3].color);
		ImGui::SliderFloat("radius3: ", &(lightParams[3]).radius, 0.f, 50.f);
		ImGui::SliderFloat("intensity3: ", &(lightParams[3]).intensity, 0.f, 50.f);
		ImGui::SliderFloat3("position3: ", lightParams[3].position, -20.f, 20.f);
		ImGui::SliderFloat("size3: ", &lightParams[3].size, 0.1f, 10.f);

		ImGui::End();
	}

	LightingPass::LightParamsGPU->FillBuffer(m_Renderer.lights.data(), sizeof(LightParams) * numOfLights);

	SetCombinedImageSampler(0, GBufferPass::Albedo);
	SetCombinedImageSampler(1, GBufferPass::Normals);
	SetCombinedImageSampler(2, GBufferPass::WorldPosition);
	SetConstantBuffer(3, m_Renderer.GetMainConstBuffer());
	SetConstantBuffer(4, LightingPass::LightParamsGPU);
	SetCombinedImageSampler(5, SSAOBlurPass::BluredOutput);
	SetCombinedImageSampler(6, RTShadowsPass::ShadowMask);
	SetCombinedImageSampler(7, GBufferPass::Velocity);
	SetCombinedImageSampler(8, GBufferPass::Depth);
	SetCombinedImageSampler(9, ShadowDenoiseFilterPass::ShadowMask);

	SetRenderTarget(0, LightingPass::MainLighting);
}

void LightingPass::Render()
{
	m_Renderer.DrawFullScreenQuad();
}