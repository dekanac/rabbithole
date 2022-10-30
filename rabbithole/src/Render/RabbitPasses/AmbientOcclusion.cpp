#include "AmbientOcclusion.h"

#include "Render/RabbitPasses/GBuffer.h"

#include <random>

defineResource(SSAOPass, Output, VulkanTexture);
defineResource(SSAOPass, Noise, VulkanTexture);
defineResource(SSAOPass, Samples, VulkanBuffer);
defineResource(SSAOPass, ParamsGPU, VulkanBuffer);
SSAOPass::SSAOParams SSAOPass::ParamsCPU = {};

defineResource(SSAOBlurPass, BluredOutput, VulkanTexture);

void SSAOPass::DeclareResources()
{
	ParamsGPU = m_Renderer.GetResourceManager().CreateBuffer(m_Renderer.GetVulkanDevice(), BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(SSAOParams)},
			.name = {"SSAO Params"}
		});

	Output = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read},
			.format = {Format::R32_SFLOAT},
			.name = {"SSAO Main"}
		});

	std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
	std::default_random_engine generator;
	std::vector<glm::vec4> ssaoKernel;
	for (unsigned int i = 0; i < 64; ++i)
	{
		glm::vec4 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator), 0.0f);
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = static_cast<float>(i) / 64.f;

		// scale samples s.t. they're more aligned to center of kernel
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssaoKernel.push_back(sample);
	}

	uint32_t bufferSize = 1024;
	Samples = m_Renderer.GetResourceManager().CreateBuffer(m_Renderer.GetVulkanDevice(), BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {bufferSize},
			.name = {"SSAO Samples"}
		});

	Samples->FillBuffer(ssaoKernel.data(), bufferSize);

	//generate SSAO Noise texture
	std::vector<glm::vec4> ssaoNoise;
	for (unsigned int i = 0; i < 16; i++)
	{
		glm::vec4 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f, 0.0f); // rotate around z-axis (in tangent space)
		ssaoNoise.push_back(noise);
	}

	TextureData texData{};

	texData.bpp = 4;
	texData.height = 4;
	texData.width = 4;
	texData.pData = (unsigned char*)ssaoNoise.data();

	Noise = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), &texData, ROTextureCreateInfo{
			.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst},
			.format = {Format::R32G32B32A32_FLOAT},
			.name = {"SSAO Noise"}
		});

	//init ssao params
	ParamsCPU.radius = 0.5f;
	ParamsCPU.bias = 0.025f;
	ParamsCPU.resWidth = static_cast<float>(GetNativeWidth);
	ParamsCPU.resHeight = static_cast<float>(GetNativeHeight);
	ParamsCPU.power = 1.75f;
	ParamsCPU.kernelSize = 48;
	ParamsCPU.ssaoOn = true;
}
void SSAOPass::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	stateManager.SetVertexShader(m_Renderer.GetShader("VS_PassThrough"));
	stateManager.SetPixelShader(m_Renderer.GetShader("FS_SSAO"));

	auto renderPassInfo = stateManager.GetRenderPassInfo();
	renderPassInfo->InitialRenderTargetState = ResourceState::None;
	renderPassInfo->FinalRenderTargetState = ResourceState::RenderTarget;
	renderPassInfo->InitialDepthStencilState = ResourceState::None;
	renderPassInfo->FinalDepthStencilState = ResourceState::None;

	auto pipelineInfo = stateManager.GetPipelineInfo();
	pipelineInfo->SetAttachmentCount(1);
	pipelineInfo->SetColorWriteMask(0, ColorWriteMaskFlags::R);

	auto& device = m_Renderer.GetVulkanDevice();

	//fill params buffer
	if (m_Renderer.imguiReady)
	{
		ImGui::Begin("SSAOParams");

		ImGui::SliderFloat("Radius: ", &ParamsCPU.radius, 0.1f, 1.f);
		ImGui::SliderFloat("Bias:", &ParamsCPU.bias, 0.0f, 0.0625f);
		ImGui::SliderFloat("Power:", &ParamsCPU.power, 1.0f, 3.f);
		ImGui::SliderInt("Kernel Size: ", &ParamsCPU.kernelSize, 1, 64);
		ImGui::Checkbox("SSSAO On: ", &ParamsCPU.ssaoOn);
		ImGui::End();
	}

	ParamsGPU->FillBuffer(&ParamsCPU, sizeof(SSAOParams));

	SetConstantBuffer(0, m_Renderer.GetMainConstBuffer());
	SetCombinedImageSampler(1, GBufferPass::WorldPosition);
	SetCombinedImageSampler(2, GBufferPass::Normals);
	SetCombinedImageSampler(3, SSAOPass::Noise);
	SetConstantBuffer(4, SSAOPass::Samples);
	SetConstantBuffer(5, SSAOPass::ParamsGPU);

	SetRenderTarget(0, SSAOPass::Output);
}
void SSAOPass::Render()
{
	m_Renderer.DrawFullScreenQuad();
}
void SSAOBlurPass::DeclareResources()
{
	BluredOutput = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read},
			.format = {Format::R32_SFLOAT},
			.name = {"SSAO Blured"}
		});
}
void SSAOBlurPass::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	stateManager.SetVertexShader(m_Renderer.GetShader("VS_PassThrough"));
	stateManager.SetPixelShader(m_Renderer.GetShader("FS_SSAOBlur"));

	auto renderPassInfo = stateManager.GetRenderPassInfo();
	renderPassInfo->InitialRenderTargetState = ResourceState::None;
	renderPassInfo->FinalRenderTargetState = ResourceState::RenderTarget;
	renderPassInfo->InitialDepthStencilState = ResourceState::None;
	renderPassInfo->FinalDepthStencilState = ResourceState::None;

	auto& device = m_Renderer.GetVulkanDevice();

	SetCombinedImageSampler(0, SSAOPass::Output);
	SetRenderTarget(0, SSAOBlurPass::BluredOutput);

}
void SSAOBlurPass::Render()
{
	m_Renderer.DrawFullScreenQuad();
}