#include "Render/Camera.h"
#include "Render/RabbitPass.h"
#include "Render/ResourceStateTracking.h"
#include "Render/SuperResolutionManager.h"

#include <random>

defineResource(GBufferPass, Albedo, VulkanTexture);
defineResource(GBufferPass, Normals, VulkanTexture);
defineResource(GBufferPass, Velocity, VulkanTexture);
defineResource(GBufferPass, WorldPosition, VulkanTexture);
defineResource(GBufferPass, Depth, VulkanTexture);

defineResource(LightingPass, MainLighting, VulkanTexture);
defineResource(LightingPass, LightParamsGPU, VulkanBuffer);

defineResource(SSAOPass, Output, VulkanTexture);
defineResource(SSAOPass, Noise, VulkanTexture);
defineResource(SSAOPass, Samples, VulkanBuffer);
defineResource(SSAOPass, ParamsGPU, VulkanBuffer);
SSAOPass::SSAOParams SSAOPass::ParamsCPU = {};
defineResource(SSAOBlurPass, BluredOutput, VulkanTexture);

defineResource(RTShadowsPass, ShadowMask, VulkanTexture);
uint32_t RTShadowsPass::ShadowResX = 0;
uint32_t RTShadowsPass::ShadowResY = 0;

defineResource(ShadowDenoisePrePass, BufferDimensions, VulkanBuffer);
defineResourceArray(ShadowDenoisePrePass, ShadowData, VulkanBuffer, MAX_NUM_OF_LIGHTS);

defineResource(ShadowDenoiseTileClassificationPass, LastFrameDepth, VulkanTexture);
defineResourceArray(ShadowDenoiseTileClassificationPass, Moments0, VulkanTexture, MAX_NUM_OF_LIGHTS);
defineResourceArray(ShadowDenoiseTileClassificationPass, Moments1, VulkanTexture, MAX_NUM_OF_LIGHTS);
defineResourceArray(ShadowDenoiseTileClassificationPass, Reprojection0, VulkanTexture, MAX_NUM_OF_LIGHTS);
defineResourceArray(ShadowDenoiseTileClassificationPass, Reprojection1, VulkanTexture, MAX_NUM_OF_LIGHTS);
defineResourceArray(ShadowDenoiseTileClassificationPass, TileMetadata, VulkanBuffer, MAX_NUM_OF_LIGHTS);
defineResource(ShadowDenoiseTileClassificationPass, ReprojectionInfo, VulkanBuffer);

defineResource(ShadowDenoiseFilterPass, FilterData, VulkanBuffer);
defineResource(ShadowDenoiseFilterPass, ShadowMask, VulkanTexture);

defineResource(TextureDebugPass, Output, VulkanTexture);
defineResource(TextureDebugPass, ParamsGPU, VulkanBuffer);
TextureDebugPass::DebugTextureParams TextureDebugPass::ParamsCPU = {};

defineResource(FSR2Pass, Output, VulkanTexture);

defineResource(VolumetricPass, MediaDensity, VulkanTexture);
defineResource(VolumetricPass, ParamsGPU, VulkanBuffer)
VolumetricPass::VolumetricFogParams VolumetricPass::ParamsCPU = {};

defineResource(ComputeScatteringPass, LightScattering, VulkanTexture);

defineResource(ApplyVolumetricFogPass, Output, VulkanTexture);

defineResource(TonemappingPass, Output, VulkanTexture);

defineResource(SkyboxPass, Main, VulkanTexture);

void RabbitPass::SetCombinedImageSampler(Renderer* renderer, int slot, VulkanTexture* texture)
{
	auto stateManager = renderer->GetStateManager();
	stateManager->UpdateResourceStage(texture);

	texture->SetShouldBeResourceState(ResourceState::GenericRead);
	RSTManager.AddResourceForTransition(texture);
	renderer->GetStateManager()->SetCombinedImageSampler(slot, texture);
}

void RabbitPass::SetSampledImage(Renderer* renderer, int slot, VulkanTexture* texture)
{
	auto stateManager = renderer->GetStateManager();
	stateManager->UpdateResourceStage(texture);

	texture->SetShouldBeResourceState(ResourceState::GenericRead);
	RSTManager.AddResourceForTransition(texture);
	renderer->GetStateManager()->SetSampledImage(slot, texture);
}

void RabbitPass::SetStorageImage(Renderer* renderer, int slot, VulkanTexture* texture)
{
	auto stateManager = renderer->GetStateManager();
	stateManager->UpdateResourceStage(texture);

	texture->SetShouldBeResourceState(ResourceState::GeneralCompute);
	RSTManager.AddResourceForTransition(texture);
	renderer->GetStateManager()->SetStorageImage(slot, texture);
}

void RabbitPass::SetConstantBuffer(Renderer* renderer, int slot, VulkanBuffer* buffer)
{
	renderer->GetStateManager()->SetConstantBuffer(slot, buffer);
}

void RabbitPass::SetStorageBuffer(Renderer* renderer, int slot, VulkanBuffer* buffer)
{
    renderer->GetStateManager()->SetStorageBuffer(slot, buffer);
}

void RabbitPass::SetSampler(Renderer* renderer, int slot, VulkanTexture* texture)
{
	renderer->GetStateManager()->SetSampler(slot, texture);
}

void RabbitPass::SetRenderTarget(Renderer* renderer, int slot, VulkanTexture* texture)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();
	stateManager->UpdateResourceStage(texture);

	texture->SetShouldBeResourceState(ResourceState::RenderTarget);
	RSTManager.AddResourceForTransition(texture);

	switch (slot)
	{
	case 0:
		stateManager->SetRenderTarget0(texture->GetView());
		break;
	case 1:
		stateManager->SetRenderTarget1(texture->GetView());
		break;
	case 2:
		stateManager->SetRenderTarget2(texture->GetView());
		break;
	case 3:
		stateManager->SetRenderTarget3(texture->GetView());
		break;
	case 4:
		stateManager->SetRenderTarget4(texture->GetView());
		break;
	default:
		ASSERT(false, "Render Target number exceeded!");
		break;
	}
}

void RabbitPass::SetDepthStencil(Renderer* renderer, VulkanTexture* texture)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();
	stateManager->UpdateResourceStage(texture);

	texture->SetShouldBeResourceState(ResourceState::DepthStencilWrite);
	RSTManager.AddResourceForTransition(texture);
	stateManager->SetDepthStencil(texture->GetView());
}

float skyboxVertices[] = {
	// positions				//for now i got only one possible vertex binding and that is 3, 3, 3, 2 TODO: clean this
	-1000.0f,  1000.0f, -1000.0f,       0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	-1000.0f, -1000.0f, -1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	 1000.0f, -1000.0f, -1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	 1000.0f, -1000.0f, -1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	 1000.0f,  1000.0f, -1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	-1000.0f,  1000.0f, -1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	-1000.0f, -1000.0f,  1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	-1000.0f, -1000.0f, -1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	-1000.0f,  1000.0f, -1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	-1000.0f,  1000.0f, -1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	-1000.0f,  1000.0f,  1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	-1000.0f, -1000.0f,  1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	 1000.0f, -1000.0f, -1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	 1000.0f, -1000.0f,  1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	 1000.0f,  1000.0f,  1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	 1000.0f,  1000.0f,  1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	 1000.0f,  1000.0f, -1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	 1000.0f, -1000.0f, -1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	-1000.0f, -1000.0f,  1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	-1000.0f,  1000.0f,  1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	 1000.0f,  1000.0f,  1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	 1000.0f,  1000.0f,  1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	 1000.0f, -1000.0f,  1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	-1000.0f, -1000.0f,  1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	-1000.0f,  1000.0f, -1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	 1000.0f,  1000.0f, -1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	 1000.0f,  1000.0f,  1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	 1000.0f,  1000.0f,  1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	-1000.0f,  1000.0f,  1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	-1000.0f,  1000.0f, -1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	-1000.0f, -1000.0f, -1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	-1000.0f, -1000.0f,  1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	 1000.0f, -1000.0f, -1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	 1000.0f, -1000.0f, -1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	-1000.0f, -1000.0f,  1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
	 1000.0f, -1000.0f,  1000.0f,		0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f
};

void GBufferPass::DeclareResources(Renderer* renderer)
{
	Albedo = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth , GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read},
			.format = {Format::B8G8R8A8_UNORM},
			.name = {"GBuffer Albedo"}
		});

	Normals = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth , GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::Storage},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"GBuffer Normal"}
		});

	WorldPosition = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth , GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::Storage},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"GBuffer World Position"}
		});

	Velocity = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth , GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::Storage},
			.format = {Format::R32G32_FLOAT},
			.name = {"GBuffer Velocity"}
		});

	Depth = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth , GetNativeHeight, 1},
			.flags = {TextureFlags::DepthStencil | TextureFlags::Read | TextureFlags::TransferSrc},
			.format = {Format::D32_SFLOAT},
			.name = {"GBuffer DepthStencil"}
		});
}

void GBufferPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetVertexShader(renderer->GetShader("VS_GBuffer"));
	stateManager->SetPixelShader(renderer->GetShader("FS_GBuffer"));

	renderer->BindViewport(0, 0, static_cast<float>(GetNativeWidth), static_cast<float>(GetNativeHeight));
	stateManager->ShouldCleanColor(true);
	stateManager->ShouldCleanDepth(true);

	auto pipelineInfo = stateManager->GetPipelineInfo();

#ifdef USE_RABBITHOLE_TOOLS
	pipelineInfo->SetAttachmentCount(5);
	pipelineInfo->SetColorWriteMask(4, ColorWriteMaskFlags::RGBA);
#else
	pipelineInfo->SetAttachmentCount(4);
#endif
	pipelineInfo->SetColorWriteMask(0, ColorWriteMaskFlags::RGBA);
	pipelineInfo->SetColorWriteMask(1, ColorWriteMaskFlags::RGBA);
	pipelineInfo->SetColorWriteMask(2, ColorWriteMaskFlags::RGBA);
	pipelineInfo->SetColorWriteMask(3, ColorWriteMaskFlags::RGBA);

	SetRenderTarget(renderer, 0, GBufferPass::Albedo);
	SetRenderTarget(renderer, 1, GBufferPass::Normals);
	SetRenderTarget(renderer, 2, GBufferPass::WorldPosition);
	SetRenderTarget(renderer, 3, GBufferPass::Velocity);
#ifdef RABBITHOLE_TOOLS
	SetRenderTarget(renderer, 4, renderer->entityHelper);
#endif
	SetDepthStencil(renderer, GBufferPass::Depth);

	auto renderPassInfo = stateManager->GetRenderPassInfo();

	stateManager->GetPipelineInfo()->SetDepthTestEnabled(true);
	renderPassInfo->InitialRenderTargetState = ResourceState::None;
	renderPassInfo->FinalRenderTargetState = ResourceState::RenderTarget;
	renderPassInfo->InitialDepthStencilState = ResourceState::None;
	renderPassInfo->FinalDepthStencilState = ResourceState::DepthStencilWrite;

	stateManager->SetCullMode(CullMode::Front);
}

void GBufferPass::Render(Renderer* renderer)
{
	renderer->CopyImage(Depth, ShadowDenoiseTileClassificationPass::LastFrameDepth);

	renderer->DrawGeometryGLTF(renderer->gltfModels);
}

void LightingPass::DeclareResources(Renderer* renderer)
{
	MainLighting = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::TransferSrc},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Lighting Main"},
		});

	LightParamsGPU = renderer->GetResourceManager()->CreateBuffer(renderer->GetVulkanDevice(), BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(LightParams) * MAX_NUM_OF_LIGHTS},
			.name = {"Light params"}
		});
}

void LightingPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetVertexShader(renderer->GetShader("VS_PassThrough"));
	stateManager->SetPixelShader(renderer->GetShader("FS_PBR"));

	//fill the light buffer
	if (renderer->imguiReady)
	{
		ImGui::Begin("Light params");

		auto& lightParams = renderer->lights;

		ImGui::Text("Sun");
		ImGui::ColorEdit3("col0: ", lightParams[0].color);
        ImGui::SliderFloat("intensity0: ", &(lightParams[0]).intensity, 0.f, 2.f);
        ImGui::SliderFloat3("position0: ", lightParams[0].position, -200.f, 200.f);
		ImGui::SliderFloat("size0: ", &lightParams[0].size, 0.1f, 10.f);
		
		ImGui::Text("Light 1");
		ImGui::ColorEdit3("col", lightParams[1].color);
		ImGui::SliderFloat("radius1: ", &(lightParams[1]).radius, 0.f, 50.f);
        ImGui::SliderFloat("intensity1: ", &(lightParams[1]).intensity, 0.f, 2.f);
		ImGui::SliderFloat3("position1: ", lightParams[1].position, -20.f, 20.f);
		ImGui::SliderFloat("size1: ", &lightParams[1].size, 0.1f, 10.f);

		ImGui::Text("Light 2");
		ImGui::ColorEdit3("col2", lightParams[2].color);
		ImGui::SliderFloat("radius2: ", &(lightParams[2]).radius, 0.f, 50.f);
        ImGui::SliderFloat("intensity2: ", &(lightParams[2]).intensity, 0.f, 2.f);
		ImGui::SliderFloat3("position2: ", lightParams[2].position, -20.f, 20.f);
		ImGui::SliderFloat("size:2 ", &lightParams[2].size, 0.1f, 10.f);

		ImGui::Text("Light 3");
		ImGui::ColorEdit3("col3", lightParams[3].color);
        ImGui::SliderFloat("radius3: ", &(lightParams[3]).radius, 0.f, 50.f);
        ImGui::SliderFloat("intensity3: ", &(lightParams[3]).intensity, 0.f, 2.f);
		ImGui::SliderFloat3("position3: ", lightParams[3].position, -20.f, 20.f);
		ImGui::SliderFloat("size3: ", &lightParams[3].size, 0.1f, 10.f);

		ImGui::End();
	}

	LightingPass::LightParamsGPU->FillBuffer(renderer->lights.data(), sizeof(LightParams) * numOfLights);

	SetCombinedImageSampler(renderer, 0, GBufferPass::Albedo);
	SetCombinedImageSampler(renderer, 1, GBufferPass::Normals);
	SetCombinedImageSampler(renderer, 2, GBufferPass::WorldPosition);
	SetConstantBuffer(renderer, 3, renderer->GetMainConstBuffer());
	SetConstantBuffer(renderer, 4, LightingPass::LightParamsGPU);
	SetCombinedImageSampler(renderer, 5, SSAOBlurPass::BluredOutput);
	SetCombinedImageSampler(renderer, 6, RTShadowsPass::ShadowMask);
	SetCombinedImageSampler(renderer, 7, GBufferPass::Velocity);
	SetCombinedImageSampler(renderer, 8, GBufferPass::Depth);
	SetCombinedImageSampler(renderer, 9, ShadowDenoiseFilterPass::ShadowMask);

	SetRenderTarget(renderer, 0, LightingPass::MainLighting);
}

void LightingPass::Render(Renderer* renderer)
{
	renderer->DrawFullScreenQuad();
}

void CopyToSwapchainPass::DeclareResources(Renderer* renderer)
{

}

void CopyToSwapchainPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();
	
	uint32_t renderWindowWidth = Window::instance().GetExtent().width;
	uint32_t renderWindowHeight = Window::instance().GetExtent().height;
	
	float renderViewportWidth = static_cast<float>(renderWindowHeight) * 1.77777f;
	float renderViewportHeight = static_cast<float>(renderWindowHeight);
	
	stateManager->SetFramebufferExtent(Extent2D{ renderWindowWidth, renderWindowHeight });
	renderer->BindViewport(0, 0, renderViewportWidth, renderViewportHeight);
	
	stateManager->SetVertexShader(renderer->GetShader("VS_PassThrough"));
	stateManager->SetPixelShader(renderer->GetShader("FS_PassThrough"));
	
	SetCombinedImageSampler(renderer, 0, TonemappingPass::Output);
	
	stateManager->SetRenderTarget0(renderer->GetSwapchainImage());
	
	auto pipelineInfo = stateManager->GetPipelineInfo();
	pipelineInfo->SetDepthTestEnabled(false);
	
	auto renderPassInfo = stateManager->GetRenderPassInfo();
	renderPassInfo->InitialRenderTargetState = ResourceState::None;
	renderPassInfo->FinalRenderTargetState = ResourceState::Present;
	renderPassInfo->InitialDepthStencilState = ResourceState::DepthStencilWrite;
	renderPassInfo->FinalDepthStencilState = ResourceState::None;
}

void CopyToSwapchainPass::Render(Renderer* renderer)
{
	renderer->CopyToSwapChain();
}

void OutlineEntityPass::DeclareResources(Renderer* renderer)
{

}

void OutlineEntityPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetVertexShader(renderer->GetShader("VS_PassThrough"));
	stateManager->SetPixelShader(renderer->GetShader("FS_OutlineEntity"));

	stateManager->ShouldCleanColor(false);

	renderer->UpdateEntityPickId();

	auto pipelineInfo = stateManager->GetPipelineInfo();
	pipelineInfo->SetAttachmentCount(1);
	pipelineInfo->SetColorWriteMask(0, ColorWriteMaskFlags::RGBA);

	auto renderPassInfo = stateManager->GetRenderPassInfo();
	renderPassInfo->InitialRenderTargetState = ResourceState::None;
	renderPassInfo->FinalRenderTargetState = ResourceState::RenderTarget;

	SetCombinedImageSampler(renderer, 0, renderer->entityHelper);
	SetRenderTarget(renderer, 0, LightingPass::MainLighting);
}

void OutlineEntityPass::Render(Renderer* renderer)
{
	renderer->DrawFullScreenQuad();
}

void SkyboxPass::DeclareResources(Renderer* renderer)
{
	Main = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), "res/textures/skybox/skybox.jpg", ROTextureCreateInfo{
			.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::CubeMap | TextureFlags::TransferDst},
			.format = {Format::R8G8B8A8_UNORM_SRGB},
			.name = {"Skybox"}
		});
}
void SkyboxPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetVertexShader(renderer->GetShader("VS_Skybox"));
	stateManager->SetPixelShader(renderer->GetShader("FS_Skybox"));
	
	stateManager->ShouldCleanDepth(false);
	stateManager->ShouldCleanColor(false);

	auto pipelineInfo = stateManager->GetPipelineInfo();
	pipelineInfo->SetDepthTestEnabled(true);

	auto renderPassInfo = stateManager->GetRenderPassInfo();
	renderPassInfo->InitialRenderTargetState = ResourceState::RenderTarget;

	stateManager->SetCullMode(CullMode::Front);

	SetConstantBuffer(renderer, 0, renderer->GetMainConstBuffer());
	SetCombinedImageSampler(renderer, 1, SkyboxPass::Main);

	SetRenderTarget(renderer, 0, GBufferPass::Albedo);
	SetDepthStencil(renderer, GBufferPass::Depth);
}
void SkyboxPass::Render(Renderer* renderer)
{
	//TODO: dont do like this, make one big DEVICE LOCAL buffer and calculate offsets
	renderer->GetVertexUploadBuffer()->FillBuffer(skyboxVertices, 396 * sizeof(float));
	renderer->BindVertexData(0);
	renderer->DrawVertices(36);
}

void SSAOPass::DeclareResources(Renderer* renderer)
{
	ParamsGPU = renderer->GetResourceManager()->CreateBuffer(renderer->GetVulkanDevice(), BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(SSAOParams)},
			.name = {"SSAO Params"}
		});

	Output = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
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
	Samples = renderer->GetResourceManager()->CreateBuffer(renderer->GetVulkanDevice(), BufferCreateInfo{
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

	Noise = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), &texData, ROTextureCreateInfo{
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
void SSAOPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetVertexShader(renderer->GetShader("VS_PassThrough"));
	stateManager->SetPixelShader(renderer->GetShader("FS_SSAO"));

	auto renderPassInfo = stateManager->GetRenderPassInfo();
	renderPassInfo->InitialRenderTargetState = ResourceState::None;
	renderPassInfo->FinalRenderTargetState = ResourceState::RenderTarget;
	renderPassInfo->InitialDepthStencilState = ResourceState::None;
	renderPassInfo->FinalDepthStencilState = ResourceState::None;
	
	auto pipelineInfo = stateManager->GetPipelineInfo();
	pipelineInfo->SetAttachmentCount(1);
	pipelineInfo->SetColorWriteMask(0, ColorWriteMaskFlags::R);

	auto& device = renderer->GetVulkanDevice();

	//fill params buffer
	if (renderer->imguiReady)
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

	SetConstantBuffer(renderer, 0, renderer->GetMainConstBuffer());
	SetCombinedImageSampler(renderer, 1, GBufferPass::WorldPosition);
	SetCombinedImageSampler(renderer, 2, GBufferPass::Normals);
	SetCombinedImageSampler(renderer, 3, SSAOPass::Noise);
    SetConstantBuffer(renderer, 4, SSAOPass::Samples);
    SetConstantBuffer(renderer, 5, SSAOPass::ParamsGPU);

	SetRenderTarget(renderer, 0, SSAOPass::Output);
}
void SSAOPass::Render(Renderer* renderer) 
{
	renderer->DrawFullScreenQuad();
}
void SSAOBlurPass::DeclareResources(Renderer* renderer)
{
	BluredOutput = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read},
			.format = {Format::R32_SFLOAT},
			.name = {"SSAO Blured"}
		});
}
void SSAOBlurPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetVertexShader(renderer->GetShader("VS_PassThrough"));
	stateManager->SetPixelShader(renderer->GetShader("FS_SSAOBlur"));

	auto renderPassInfo = stateManager->GetRenderPassInfo();
	renderPassInfo->InitialRenderTargetState = ResourceState::None;
	renderPassInfo->FinalRenderTargetState = ResourceState::RenderTarget;
	renderPassInfo->InitialDepthStencilState = ResourceState::None;
	renderPassInfo->FinalDepthStencilState = ResourceState::None;

	auto& device = renderer->GetVulkanDevice();

	SetCombinedImageSampler(renderer, 0, SSAOPass::Output);
	SetRenderTarget(renderer, 0, SSAOBlurPass::BluredOutput);

}
void SSAOBlurPass::Render(Renderer* renderer)
{
	renderer->DrawFullScreenQuad();
}

void RTShadowsPass::DeclareResources(Renderer* renderer) 
{
	const float shadowResolutionMultiplier = 1.f;

	ShadowResX = static_cast<uint32_t>(GetNativeWidth * shadowResolutionMultiplier);
	ShadowResY = static_cast<uint32_t>(GetNativeHeight * shadowResolutionMultiplier);

	ShadowMask = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {ShadowResX, ShadowResY, 1},
			.flags = {TextureFlags::Read | TextureFlags::Storage},
			.format = {Format::R8_UNORM},
			.name = {"Shadow Mask"},
			.arraySize = {MAX_NUM_OF_LIGHTS},
		});
}

void RTShadowsPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetComputeShader(renderer->GetShader("CS_RayTracingShadows"));

	SetStorageImage(renderer, 0, GBufferPass::WorldPosition);
	SetStorageImage(renderer, 1, GBufferPass::Normals);
	SetStorageImage(renderer, 2, RTShadowsPass::ShadowMask);
	SetStorageBuffer(renderer, 3, renderer->vertexBuffer);
	SetStorageBuffer(renderer, 4, renderer->trianglesBuffer);
	SetStorageBuffer(renderer, 5, renderer->triangleIndxsBuffer);
	SetStorageBuffer(renderer, 6, renderer->cfbvhNodesBuffer);
	SetConstantBuffer(renderer, 7, LightingPass::LightParamsGPU);
	SetStorageImage(renderer, 8, renderer->blueNoise2DTexture);
	SetConstantBuffer(renderer, 9, renderer->GetMainConstBuffer());
}

void RTShadowsPass::Render(Renderer* renderer)
{
	static const int threadGroupWorkRegionDim = 8;

	int texWidth = RTShadowsPass::ShadowMask->GetWidth();
	int texHeight = RTShadowsPass::ShadowMask->GetHeight();

	int dispatchX = GetCSDispatchCount(texWidth, threadGroupWorkRegionDim);
	int dispatchY = GetCSDispatchCount(texHeight, threadGroupWorkRegionDim);

	renderer->Dispatch(dispatchX, dispatchY, 1);
}

void FSR2Pass::DeclareResources(Renderer* renderer)
{
	Output = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetUpscaledWidth, GetUpscaledHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::Storage},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"FSR2 Output"},
		});
}

void FSR2Pass::Setup(Renderer* renderer)
{
	renderer->ResourceBarrier(FSR2Pass::Output, ResourceState::GenericRead, ResourceState::GeneralCompute, ResourceStage::Graphics, ResourceStage::Compute);
	renderer->ResourceBarrier(ApplyVolumetricFogPass::Output, ResourceState::RenderTarget, ResourceState::GenericRead, ResourceStage::Graphics, ResourceStage::Compute);
}

void FSR2Pass::Render(Renderer* renderer)
{
	SuperResolutionManager::FfxUpscaleSetup fsrSetup{};

	const auto& cameraState = renderer->GetCameraState();

	fsrSetup.cameraSetup.cameraPos = rabbitVec4f{ cameraState.CameraPosition, 1.0f };
	fsrSetup.cameraSetup.cameraProj = cameraState.ProjectionMatrix;
	fsrSetup.cameraSetup.cameraView = cameraState.ViewMatrix;
	fsrSetup.cameraSetup.cameraViewInv = cameraState.ViewInverseMatrix;

	fsrSetup.depthbufferResource = GBufferPass::Depth;
	fsrSetup.motionvectorResource = GBufferPass::Velocity;
	fsrSetup.unresolvedColorResource = ApplyVolumetricFogPass::Output;
	fsrSetup.resolvedColorResource = FSR2Pass::Output;

	SuperResolutionManager::instance().Draw(renderer->GetCurrentCommandBuffer(), fsrSetup, &renderer->GetUIState());

	renderer->ResourceBarrier(FSR2Pass::Output, ResourceState::GeneralCompute, ResourceState::GenericRead, ResourceStage::Compute, ResourceStage::Graphics);
}

void VolumetricPass::DeclareResources(Renderer* renderer)
{
	MediaDensity = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {160, 90, 128},
			.flags = {TextureFlags::Read | TextureFlags::TransferSrc | TextureFlags::Storage},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Media Density"},
		});

	ParamsGPU = renderer->GetResourceManager()->CreateBuffer(renderer->GetVulkanDevice(), BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(VolumetricFogParams)},
			.name = {"Volumetric Fog Params"}
		});
}

void VolumetricPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetComputeShader(renderer->GetShader("CS_Volumetric"));

	SetStorageImage(renderer, 0, VolumetricPass::MediaDensity);
	SetCombinedImageSampler(renderer, 1, renderer->noise3DLUT);
	SetConstantBuffer(renderer, 2, LightingPass::LightParamsGPU);
	SetConstantBuffer(renderer, 3, renderer->GetMainConstBuffer());
	SetConstantBuffer(renderer, 4, VolumetricPass::ParamsGPU);

	SetStorageBuffer(renderer, 5, renderer->vertexBuffer);
	SetStorageBuffer(renderer, 6, renderer->trianglesBuffer);
	SetStorageBuffer(renderer, 7, renderer->triangleIndxsBuffer);
	SetStorageBuffer(renderer, 8, renderer->cfbvhNodesBuffer);

	if (renderer->imguiReady)
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

void VolumetricPass::Render(Renderer* renderer)
{
	int texWidth = VolumetricPass::MediaDensity->GetWidth();
	int texHeight = VolumetricPass::MediaDensity->GetHeight();
	int texDepth = VolumetricPass::MediaDensity->GetDepth();

	int dispatchX = (texWidth + (8 - 1)) / 8;
	int dispatchY = (texHeight + (4 - 1)) / 4;
	int dispatchZ = (texDepth + (8 - 1)) / 8;

	renderer->Dispatch(dispatchX, dispatchY, dispatchZ);
}

void Create3DNoiseTexturePass::DeclareResources(Renderer* renderer)
{
}

void Create3DNoiseTexturePass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetComputeShader(renderer->GetShader("CS_3DNoiseLUT"));

	SetCombinedImageSampler(renderer, 0, renderer->noise2DTexture);
	SetStorageImage(renderer, 1, renderer->noise3DLUT);
}

void Create3DNoiseTexturePass::Render(Renderer* renderer)
{
	renderer->Dispatch(256, 256, 256);
}


void ComputeScatteringPass::DeclareResources(Renderer* renderer)
{
	LightScattering = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {160, 90, 64},
			.flags = {TextureFlags::Read | TextureFlags::TransferSrc | TextureFlags::Storage},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Scattering Calculation"},
		});
}


void ComputeScatteringPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetComputeShader(renderer->GetShader("CS_ComputeScattering"));

	SetStorageImage(renderer, 0, VolumetricPass::MediaDensity);
	SetStorageImage(renderer, 1, ComputeScatteringPass::LightScattering);
	SetConstantBuffer(renderer, 2, VolumetricPass::ParamsGPU);
}


void ComputeScatteringPass::Render(Renderer* renderer)
{
	int texWidth = ComputeScatteringPass::LightScattering->GetWidth();
	int texHeight = ComputeScatteringPass::LightScattering->GetHeight();

	static const int threadGroupWorkRegionDim = 8;
	int dispatchX = (texWidth + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
	int dispatchY = (texHeight + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;

	renderer->Dispatch(dispatchX, dispatchY, 1);
}

void ApplyVolumetricFogPass::DeclareResources(Renderer* renderer)
{
	Output = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::TransferSrc},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Volumetric Fog Output"},
		});
}

void ApplyVolumetricFogPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetVertexShader(renderer->GetShader("VS_PassThrough"));
	stateManager->SetPixelShader(renderer->GetShader("FS_ApplyVolumetricFog"));

	SetCombinedImageSampler(renderer, 0, LightingPass::MainLighting);
	SetCombinedImageSampler(renderer, 1, GBufferPass::Depth);
	SetCombinedImageSampler(renderer, 2, ComputeScatteringPass::LightScattering);
	SetConstantBuffer(renderer, 3, renderer->GetMainConstBuffer());
	SetConstantBuffer(renderer, 4, VolumetricPass::ParamsGPU);

	SetRenderTarget(renderer, 0, ApplyVolumetricFogPass::Output);
}

void ApplyVolumetricFogPass::Render(Renderer* renderer)
{
	renderer->DrawFullScreenQuad();
}

void TonemappingPass::DeclareResources(Renderer* renderer)
{
	Output = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetUpscaledWidth, GetUpscaledHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Post Upscale PostEffects" },
		});
}

void TonemappingPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();
	
	stateManager->SetFramebufferExtent(Extent2D{ GetUpscaledWidth , GetUpscaledHeight });
	renderer->BindViewport(0, 0, static_cast<float>(GetUpscaledWidth), static_cast<float>(GetUpscaledHeight));

	stateManager->SetVertexShader(renderer->GetShader("VS_PassThrough"));
	stateManager->SetPixelShader(renderer->GetShader("FS_Tonemap"));

	SetCombinedImageSampler(renderer, 0, FSR2Pass::Output);

	SetRenderTarget(renderer, 0, TonemappingPass::Output);
}

void TonemappingPass::Render(Renderer* renderer)
{
	renderer->DrawFullScreenQuad();
}

void TextureDebugPass::DeclareResources(Renderer* renderer)
{
	Output = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
		.dimensions = {GetNativeWidth, GetNativeHeight, 1},
		.flags = {TextureFlags::RenderTarget | TextureFlags::Read},
		.format = {Format::R16G16B16A16_FLOAT},
		.name = {"Debug Texture"}
		});

	ParamsGPU = renderer->GetResourceManager()->CreateBuffer(renderer->GetVulkanDevice(), BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(DebugTextureParams)},
			.name = {"Debug Texture Params"}
		});
}

void TextureDebugPass::Setup(Renderer* renderer)
{
    VulkanStateManager* stateManager = renderer->GetStateManager();

    stateManager->SetVertexShader(renderer->GetShader("VS_PassThrough"));
    stateManager->SetPixelShader(renderer->GetShader("FS_TextureDebug"));

	//see what texture is selected

    SetConstantBuffer(renderer, 0, TextureDebugPass::ParamsGPU);

	auto textureToBind = renderer->g_DefaultWhiteTexture;
	auto texture3DToBind = renderer->g_Default3DTexture;
	auto textureArrayToBind = renderer->g_DefaultArrayTexture;

	VulkanTexture* selectedTexture = renderer->GetTextureWithID(renderer->currentTextureSelectedID);

	if (selectedTexture)
	{
		auto& debugParams = TextureDebugPass::ParamsCPU;
		if (debugParams.isArray)
		{
			textureArrayToBind = selectedTexture;
		}
		else if (debugParams.is3D)
		{
			texture3DToBind = selectedTexture;
		}
		else
		{
			textureToBind = selectedTexture;
		}
	}

	SetCombinedImageSampler(renderer, 1, textureToBind);
	SetCombinedImageSampler(renderer, 2, textureArrayToBind);
	SetCombinedImageSampler(renderer, 3, texture3DToBind);

    TextureDebugPass::ParamsGPU->FillBuffer(&TextureDebugPass::ParamsCPU);

    SetRenderTarget(renderer, 0, TextureDebugPass::Output);

}

void TextureDebugPass::Render(Renderer* renderer)
{
	renderer->DrawFullScreenQuad();
	
	renderer->ResourceBarrier(TextureDebugPass::Output, ResourceState::RenderTarget, ResourceState::GenericRead, ResourceStage::Graphics, ResourceStage::Graphics);
}

void ShadowDenoisePrePass::DeclareResources(Renderer* renderer)
{
	const uint32_t tileW = GetCSDispatchCount(RTShadowsPass::ShadowResX, 8);
	const uint32_t tileH = GetCSDispatchCount(RTShadowsPass::ShadowResY, 4);

	const uint32_t tileSize = tileH * tileW;

	BufferDimensions = renderer->GetResourceManager()->CreateBuffer(renderer->GetVulkanDevice(), BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(DenoiseBufferDimensions)},
			.name = {"Denoise Dimensions buffer"}
		});

	for (uint32_t i = 0; i < MAX_NUM_OF_LIGHTS; i++)
	{
		ShadowData[i] = renderer->GetResourceManager()->CreateBuffer(renderer->GetVulkanDevice(), BufferCreateInfo{
				.flags = {BufferUsageFlags::StorageBuffer},
				.memoryAccess = {MemoryAccess::GPU},
				.size = {tileSize * static_cast<uint32_t>(sizeof(uint32_t))},
				.name = {std::format("Denoise Shadow Mask Buffer {} slice", i)}
			});
	}
}

void ShadowDenoisePrePass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetComputeShader(renderer->GetShader("CS_PrepareShadowMask"));

	DenoiseBufferDimensions bufferDim{};
	bufferDim.dimensions[0] = RTShadowsPass::ShadowResX;
	bufferDim.dimensions[1] = RTShadowsPass::ShadowResY;

	ShadowDenoisePrePass::BufferDimensions->FillBuffer(&bufferDim);
}

void ShadowDenoisePrePass::Render(Renderer* renderer)
{
	for (int i = 0; i < MAX_NUM_OF_LIGHTS; i++)
		PrepareDenoisePass(renderer, i);
}

void ShadowDenoisePrePass::PrepareDenoisePass(Renderer* renderer, uint32_t shadowSlice)
{
	renderer->BindPushConstant(shadowSlice);

	SetConstantBuffer(renderer, 0, ShadowDenoisePrePass::BufferDimensions);
	SetSampledImage(renderer, 1, RTShadowsPass::ShadowMask);
	SetStorageBuffer(renderer, 2, ShadowDenoisePrePass::ShadowData[shadowSlice]);

	constexpr uint32_t threadGroupWorkRegionDimX = 8;
	constexpr uint32_t threadGroupWorkRegionDimY = 4;

	int dispatchX = GetCSDispatchCount(RTShadowsPass::ShadowResX, threadGroupWorkRegionDimX * 4);
	int dispatchY = GetCSDispatchCount(RTShadowsPass::ShadowResY, threadGroupWorkRegionDimY * 4);

	renderer->Dispatch(dispatchX, dispatchY, 1);
}

void ShadowDenoiseTileClassificationPass::DeclareResources(Renderer* renderer)
{
	const uint32_t tileW = GetCSDispatchCount(RTShadowsPass::ShadowResX, 8);
	const uint32_t tileH = GetCSDispatchCount(RTShadowsPass::ShadowResY, 4);

	const uint32_t tileSize = tileH * tileW;

	LastFrameDepth = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {RTShadowsPass::ShadowResX, RTShadowsPass::ShadowResY, 1},
			.flags = {TextureFlags::DepthStencil | TextureFlags::Read | TextureFlags::TransferDst },
			.format = {Format::D32_SFLOAT},
			.name = {"Denoise Last Frame Depth"}
		});

	ReprojectionInfo = renderer->GetResourceManager()->CreateBuffer(renderer->GetVulkanDevice(), BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(DenoiseShadowData)},
			.name = {"Denoise Shadow Data Buffer"}
		});

	for (uint32_t i = 0; i < MAX_NUM_OF_LIGHTS; i++)
	{

		TileMetadata[i] = renderer->GetResourceManager()->CreateBuffer(renderer->GetVulkanDevice(), BufferCreateInfo{
				.flags = {BufferUsageFlags::StorageBuffer},
				.memoryAccess = {MemoryAccess::GPU},
				.size = {tileSize * static_cast<uint32_t>(sizeof(uint32_t))},
				.name = {std::format("Denoise Tile Metadata Buffer {} slice", i)}
			});

		//classify
		Moments0[i] = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
				.dimensions = {RTShadowsPass::ShadowResX, RTShadowsPass::ShadowResY, 1},
				.flags = {TextureFlags::Read | TextureFlags::Storage},
				.format = {Format::R11G11B10_FLOAT},
				.name = {std::format("Denoise Moments Buffer0 {} slice", i)}
			});

		Moments1[i] = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
				.dimensions = {RTShadowsPass::ShadowResX, RTShadowsPass::ShadowResY, 1},
				.flags = {TextureFlags::Read | TextureFlags::Storage},
				.format = {Format::R11G11B10_FLOAT},
				.name = {std::format("Denoise Moments Buffer1 {} slice", i)}
			});

		Reprojection0[i] = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
				.dimensions = {RTShadowsPass::ShadowResX, RTShadowsPass::ShadowResY, 1},
				.flags = {TextureFlags::Read | TextureFlags::Storage},
				.format = {Format::R16G16_FLOAT},
				.name = {std::format("Denoise Reprojection Buffer0 {} slice", i)}
			});

		Reprojection1[i] = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
				.dimensions = {RTShadowsPass::ShadowResX, RTShadowsPass::ShadowResY, 1},
				.flags = {TextureFlags::Read | TextureFlags::Storage},
				.format = {Format::R16G16_FLOAT},
				.name = {std::format("Denoise Reprojection Buffer1 {} slice", i)}
			});
	}
}

void ShadowDenoiseTileClassificationPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetComputeShader(renderer->GetShader("CS_TileClassification"));

	CameraState& cameraState = renderer->GetCameraState();

	DenoiseShadowData shadowData{};
	shadowData.Eye = cameraState.CameraPosition;
	shadowData.FirstFrame = (int)(renderer->GetCurrentFrameIndex() == 0);
	shadowData.BufferDimensions[0] = RTShadowsPass::ShadowResX;
	shadowData.BufferDimensions[1] = RTShadowsPass::ShadowResY;
	shadowData.InvBufferDimensions[0] = 1.f / float(RTShadowsPass::ShadowResX);
	shadowData.InvBufferDimensions[1] = 1.f / float(RTShadowsPass::ShadowResY);
	shadowData.ProjectionInverse = cameraState.ProjectionInverseMatrix;
	shadowData.ViewProjectionInverse = cameraState.ViewProjInverseMatrix;
	shadowData.ReprojectionMatrix = cameraState.ProjectionMatrix * (cameraState.PrevViewMatrix * cameraState.ViewProjInverseMatrix);
	ReprojectionInfo->FillBuffer(&shadowData);
}

void ShadowDenoiseTileClassificationPass::Render(Renderer* renderer)
{
	for (int i = 0; i < MAX_NUM_OF_LIGHTS; i++)
		ClassifyTiles(renderer, i);
}

void ShadowDenoiseTileClassificationPass::ClassifyTiles(Renderer* renderer, uint32_t shadowSlice)
{
	SetConstantBuffer(renderer, 0, ShadowDenoiseTileClassificationPass::ReprojectionInfo);
	SetSampledImage(renderer, 1, GBufferPass::Depth);
	SetSampledImage(renderer, 2, GBufferPass::Velocity);
	SetSampledImage(renderer, 3, GBufferPass::Normals);
	SetSampledImage(renderer, 4, ShadowDenoiseTileClassificationPass::Reprojection1[shadowSlice]);
	SetSampledImage(renderer, 5, ShadowDenoiseTileClassificationPass::LastFrameDepth);
	SetStorageBuffer(renderer, 6, ShadowDenoisePrePass::ShadowData[shadowSlice]);
	SetStorageBuffer(renderer, 7, ShadowDenoiseTileClassificationPass::TileMetadata[shadowSlice]);
	SetStorageImage(renderer, 8, ShadowDenoiseTileClassificationPass::Reprojection0[shadowSlice]);
	SetSampledImage(renderer, 9, GetCurrentIDFromFrameIndex(0) ? ShadowDenoiseTileClassificationPass::Moments0[shadowSlice] : ShadowDenoiseTileClassificationPass::Moments1[shadowSlice]);
	SetStorageImage(renderer, 10, GetCurrentIDFromFrameIndex(1) ? ShadowDenoiseTileClassificationPass::Moments0[shadowSlice] : ShadowDenoiseTileClassificationPass::Moments1[shadowSlice]);
	SetSampler(renderer, 11, ShadowDenoiseFilterPass::ShadowMask);

	constexpr uint32_t threadGroupWorkRegionDim = 8;

	int dispatchX = GetCSDispatchCount(RTShadowsPass::ShadowResX, threadGroupWorkRegionDim);
	int dispatchY = GetCSDispatchCount(RTShadowsPass::ShadowResY, threadGroupWorkRegionDim);

	renderer->Dispatch(dispatchX, dispatchY, 1);
}

void ShadowDenoiseFilterPass::DeclareResources(Renderer* renderer)
{
	FilterData = renderer->GetResourceManager()->CreateBuffer(renderer->GetVulkanDevice(), BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(DenoiseShadowFilterData)},
			.name = {"Denoise Shadow Filter Data Buffer"}
		});

	ShadowMask = renderer->GetResourceManager()->CreateTexture(renderer->GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {RTShadowsPass::ShadowResX, RTShadowsPass::ShadowResY, 1},
			.flags = {TextureFlags::Read | TextureFlags::Storage},
			.format = {Format::R16G16B16A16_UNORM},
			.name = {"Shadow Mask Denoised"},
			.arraySize = {MAX_NUM_OF_LIGHTS},
			.isCube = { false },
			.multisampleType = {MultisampleType::Sample_1},
			.samplerType = { SamplerType::Trilinear },
			.addressMode = { AddressMode::Clamp }
		});
}

void ShadowDenoiseFilterPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();
	CameraState& cameraState = renderer->GetCameraState();

	DenoiseShadowFilterData shadowFilterData{};
	shadowFilterData.ProjectionInverse = cameraState.ProjectionInverseMatrix;
	shadowFilterData.DepthSimilaritySigma = 1.f;
	shadowFilterData.BufferDimensions[0] = static_cast<int>(GetNativeWidth);
	shadowFilterData.BufferDimensions[1] = static_cast<int>(GetNativeHeight);
	shadowFilterData.InvBufferDimensions[0] = 1.f / float(GetNativeWidth);
	shadowFilterData.InvBufferDimensions[1] = 1.f / float(GetNativeHeight);

	ShadowDenoiseFilterPass::FilterData->FillBuffer(&shadowFilterData);
}

void ShadowDenoiseFilterPass::Render(Renderer* renderer)
{
	for (uint32_t i = 0; i < MAX_NUM_OF_LIGHTS; i++)
	{
		RenderFilterPass0(renderer, i);
		RenderFilterPass1(renderer, i);
		RenderFilterPass2(renderer, i);
	}
}

void ShadowDenoiseFilterPass::RenderFilterPass0(Renderer* renderer, uint32_t shadowSlice)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetComputeShader(renderer->GetShader("CS_FilterSoftShadowsPass0"), "Pass0");

	SetConstantBuffer(renderer, 0, ShadowDenoiseFilterPass::FilterData);
	SetSampledImage(renderer, 1, GBufferPass::Depth);
	SetSampledImage(renderer, 2, GBufferPass::Normals);
	SetStorageBuffer(renderer, 3, ShadowDenoiseTileClassificationPass::TileMetadata[shadowSlice]);
	SetSampledImage(renderer, 4, ShadowDenoiseTileClassificationPass::Reprojection0[shadowSlice]);
	SetStorageImage(renderer, 5, ShadowDenoiseTileClassificationPass::Reprojection1[shadowSlice]);

	constexpr uint32_t threadGroupWorkRegionDim = 8;

	uint32_t dispatchX = GetCSDispatchCount(RTShadowsPass::ShadowResX, threadGroupWorkRegionDim);
	uint32_t dispatchY = GetCSDispatchCount(RTShadowsPass::ShadowResY, threadGroupWorkRegionDim);

	renderer->Dispatch(dispatchX, dispatchY, 1);
}

void ShadowDenoiseFilterPass::RenderFilterPass1(Renderer* renderer, uint32_t shadowSlice)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetComputeShader(renderer->GetShader("CS_FilterSoftShadowsPass1"), "Pass1");

	SetConstantBuffer(renderer, 0, ShadowDenoiseFilterPass::FilterData);
	SetSampledImage(renderer, 1, GBufferPass::Depth);
	SetSampledImage(renderer, 2, GBufferPass::Normals);
	SetStorageBuffer(renderer, 3, ShadowDenoiseTileClassificationPass::TileMetadata[shadowSlice]);
	SetSampledImage(renderer, 4, ShadowDenoiseTileClassificationPass::Reprojection1[shadowSlice]);
	SetStorageImage(renderer, 5, ShadowDenoiseTileClassificationPass::Reprojection0[shadowSlice]);

	constexpr uint32_t threadGroupWorkRegionDim = 8;

	uint32_t dispatchX = GetCSDispatchCount(RTShadowsPass::ShadowResX, threadGroupWorkRegionDim);
	uint32_t dispatchY = GetCSDispatchCount(RTShadowsPass::ShadowResY, threadGroupWorkRegionDim);

	renderer->Dispatch(dispatchX, dispatchY, 1);
}

void ShadowDenoiseFilterPass::RenderFilterPass2(Renderer* renderer, uint32_t shadowSlice)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetComputeShader(renderer->GetShader("CS_FilterSoftShadowsPass2"), "Pass2");

	renderer->BindPushConstant(shadowSlice);

	SetConstantBuffer(renderer, 0, ShadowDenoiseFilterPass::FilterData);
	SetSampledImage(renderer, 1, GBufferPass::Depth);
	SetSampledImage(renderer, 2, GBufferPass::Normals);
	SetStorageBuffer(renderer, 3, ShadowDenoiseTileClassificationPass::TileMetadata[shadowSlice]);
	SetSampledImage(renderer, 4, ShadowDenoiseTileClassificationPass::Reprojection0[shadowSlice]);
	SetStorageImage(renderer, 6, ShadowDenoiseFilterPass::ShadowMask);

	constexpr uint32_t threadGroupWorkRegionDim = 8;

	uint32_t dispatchX = GetCSDispatchCount(RTShadowsPass::ShadowResX, threadGroupWorkRegionDim);
	uint32_t dispatchY = GetCSDispatchCount(RTShadowsPass::ShadowResY, threadGroupWorkRegionDim);

	renderer->Dispatch(dispatchX, dispatchY, 1);
}

void RabbitPassManager::SchedulePasses()
{
	AddPass(new Create3DNoiseTexturePass, true);
	AddPass(new GBufferPass);
	AddPass(new SSAOPass);
	AddPass(new SSAOBlurPass);
	AddPass(new RTShadowsPass);
	AddPass(new ShadowDenoisePrePass);
	AddPass(new ShadowDenoiseTileClassificationPass);
	AddPass(new ShadowDenoiseFilterPass);
	AddPass(new VolumetricPass);
	AddPass(new ComputeScatteringPass);
	AddPass(new SkyboxPass);
	AddPass(new LightingPass);
	AddPass(new ApplyVolumetricFogPass);
	AddPass(new TextureDebugPass);
	AddPass(new FSR2Pass);
	AddPass(new TonemappingPass);
	AddPass(new CopyToSwapchainPass);
}

void RabbitPassManager::DeclareResources(Renderer* renderer)
{
	for (auto pass : m_RabbitPassesToExecute)
	{
		pass->DeclareResources(renderer);
	}
}

void RabbitPassManager::ExecutePasses(Renderer* renderer)
{
	for (auto pass : m_RabbitPassesToExecute)
	{
		renderer->BeginLabel(pass->GetName());

		pass->Setup(renderer);

		pass->Render(renderer);

		renderer->RecordGPUTimeStamp(pass->GetName());

		renderer->EndLabel();		
	}
}

void RabbitPassManager::ExecuteOneTimePasses(Renderer* renderer)
{
	for (auto pass : m_RabbitPassesOneTimeExecute)
	{
		renderer->BeginLabel(pass->GetName());

		pass->Setup(renderer);

		pass->Render(renderer);

		renderer->RecordGPUTimeStamp(pass->GetName());

		renderer->EndLabel();
	}
}

void RabbitPassManager::Destroy()
{

}
