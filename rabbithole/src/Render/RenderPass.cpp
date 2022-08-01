#include "Camera.h"
#include "RenderPass.h"
#include "Renderer.h"
#include "ResourceStateTracking.h"
#include "SuperResolutionManager.h"

void RenderPass::SetCombinedImageSampler(Renderer* renderer, int slot, VulkanTexture* texture)
{
	auto stateManager = renderer->GetStateManager();
	stateManager->UpdateResourceStage(texture);

	texture->SetShouldBeResourceState(ResourceState::GenericRead);
	RSTManager.AddResourceForTransition(texture);
	renderer->GetStateManager()->SetCombinedImageSampler(slot, texture);
}

void RenderPass::SetSampledImage(Renderer* renderer, int slot, VulkanTexture* texture)
{
	auto stateManager = renderer->GetStateManager();
	stateManager->UpdateResourceStage(texture);

	texture->SetShouldBeResourceState(ResourceState::GenericRead);
	RSTManager.AddResourceForTransition(texture);
	renderer->GetStateManager()->SetSampledImage(slot, texture);
}

void RenderPass::SetStorageImage(Renderer* renderer, int slot, VulkanTexture* texture)
{
	auto stateManager = renderer->GetStateManager();
	stateManager->UpdateResourceStage(texture);

	texture->SetShouldBeResourceState(ResourceState::GeneralCompute);
	RSTManager.AddResourceForTransition(texture);
	renderer->GetStateManager()->SetStorageImage(slot, texture);
}

void RenderPass::SetConstantBuffer(Renderer* renderer, int slot, VulkanBuffer* buffer)
{
	renderer->GetStateManager()->SetConstantBuffer(slot, buffer);
}

void RenderPass::SetStorageBuffer(Renderer* renderer, int slot, VulkanBuffer* buffer)
{
    renderer->GetStateManager()->SetStorageBuffer(slot, buffer);
}

void RenderPass::SetSampler(Renderer* renderer, int slot, VulkanTexture* texture)
{
	renderer->GetStateManager()->SetSampler(slot, texture);
}

void RenderPass::SetRenderTarget(Renderer* renderer, int slot, VulkanTexture* texture)
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

void RenderPass::SetDepthStencil(Renderer* renderer, VulkanTexture* texture)
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

void BoundingBoxPass::DeclareResources(Renderer* renderer)
{

}

void BoundingBoxPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetVertexShader(renderer->GetShader("VS_SimpleGeometry"));
	stateManager->SetPixelShader(renderer->GetShader("FS_SimpleGeometry"));

	renderer->BindViewport(0, 0, GetNativeWidth, GetNativeHeight);
	stateManager->ShouldCleanColor(false);
	stateManager->ShouldCleanDepth(false);

	auto pipelineInfo = stateManager->GetPipelineInfo();
	pipelineInfo->SetTopology(Topology::LineList);
	pipelineInfo->SetCullMode(CullMode::None);

	SetConstantBuffer(renderer, 0, renderer->GetMainConstBuffer());

	SetRenderTarget(renderer, 0, renderer->lightingMain);
	SetDepthStencil(renderer, renderer->depthStencil);

	auto renderPassInfo = stateManager->GetRenderPassInfo();

	stateManager->GetPipelineInfo()->SetDepthTestEnabled(true);
	renderPassInfo->InitialRenderTargetState = ResourceState::RenderTarget;
	renderPassInfo->FinalRenderTargetState = ResourceState::RenderTarget;
	renderPassInfo->InitialDepthStencilState = ResourceState::DepthStencilWrite;
	renderPassInfo->FinalDepthStencilState = ResourceState::DepthStencilWrite;

}

void BoundingBoxPass::Render(Renderer* renderer)
{
	//renderer->DrawBoundingBoxes(renderer->GetModels());
}

void GBufferPass::DeclareResources(Renderer* renderer)
{

}

void GBufferPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetVertexShader(renderer->GetShader("VS_GBuffer"));
	stateManager->SetPixelShader(renderer->GetShader("FS_GBuffer"));

	renderer->BindViewport(0, 0, GetNativeWidth, GetNativeHeight);
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

	SetRenderTarget(renderer, 0, renderer->albedoGBuffer);
	SetRenderTarget(renderer, 1, renderer->normalGBuffer);
	SetRenderTarget(renderer, 2, renderer->worldPositionGBuffer);
	SetRenderTarget(renderer, 3, renderer->velocityGBuffer);
#ifdef RABBITHOLE_TOOLS
	SetRenderTarget(renderer, 4, renderer->entityHelper);
#endif
	SetDepthStencil(renderer, renderer->depthStencil);

	auto renderPassInfo = stateManager->GetRenderPassInfo();

	stateManager->GetPipelineInfo()->SetDepthTestEnabled(true);
	renderPassInfo->InitialRenderTargetState = ResourceState::None;
	renderPassInfo->FinalRenderTargetState = ResourceState::RenderTarget;
	renderPassInfo->InitialDepthStencilState = ResourceState::None;
	renderPassInfo->FinalDepthStencilState = ResourceState::DepthStencilWrite;

	stateManager->SetCullMode(CullMode::Front);

	if (renderer->m_RenderTAA)
	{
		static uint32_t Seed;
		renderer->GetCamera()->SetProjectionJitter(GetNativeWidth, GetNativeHeight, Seed);
	}

	renderer->BindCameraMatrices(renderer->GetCamera());
}

void GBufferPass::Render(Renderer* renderer)
{
	//renderer->DrawGeometry(renderer->GetModels());
	renderer->DrawGeometryGLTF(renderer->gltfModels);
}

void LightingPass::DeclareResources(Renderer* renderer)
{

}

void LightingPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetVertexShader(renderer->GetShader("VS_PassThrough"));
	stateManager->SetPixelShader(renderer->GetShader("FS_PBR"));

	stateManager->ShouldCleanColor(true);

	//fill the light buffer
	if (renderer->imguiReady)
	{
		ImGui::Begin("Light params");

		auto& lightParams = renderer->lightParams;

		ImGui::ColorEdit3("SunColor: ", lightParams[0].color);
        ImGui::SliderFloat("Sun intensity: ", &(lightParams[0]).intensity, 0.f, 2.f);
        ImGui::SliderFloat3("Sun position: ", lightParams[0].position, -200.f, 200.f);

		ImGui::ColorEdit3("Light2", lightParams[1].color);
		ImGui::SliderFloat("Light2 radius: ", &(lightParams[1]).radius, 0.f, 50.f);
        ImGui::SliderFloat("Light2 intensity: ", &(lightParams[1]).intensity, 0.f, 2.f);
		ImGui::SliderFloat3("Light2 position: ", lightParams[1].position, -20.f, 20.f);

		ImGui::ColorEdit3("Light3", lightParams[2].color);
		ImGui::SliderFloat("Light3 radius: ", &(lightParams[2]).radius, 0.f, 50.f);
        ImGui::SliderFloat("Light3 intensity: ", &(lightParams[2]).intensity, 0.f, 2.f);
		ImGui::SliderFloat3("Light3 position: ", lightParams[2].position, -20.f, 20.f);

		ImGui::ColorEdit3("Light4", lightParams[3].color);
        ImGui::SliderFloat("Light4 radius: ", &(lightParams[3]).radius, 0.f, 50.f);
        ImGui::SliderFloat("Light4 intensity: ", &(lightParams[3]).intensity, 0.f, 2.f);
		ImGui::SliderFloat3("Light4 position: ", lightParams[3].position, -20.f, 20.f);

		ImGui::End();
	}
	renderer->GetLightParams()->FillBuffer(renderer->lightParams, sizeof(LightParams) * numOfLights);

	SetCombinedImageSampler(renderer, 0, renderer->albedoGBuffer);
	SetCombinedImageSampler(renderer, 1, renderer->normalGBuffer);
	SetCombinedImageSampler(renderer, 2, renderer->worldPositionGBuffer);
	SetConstantBuffer(renderer, 3, renderer->GetMainConstBuffer());
	SetConstantBuffer(renderer, 4, renderer->GetLightParams());
	SetCombinedImageSampler(renderer, 5, renderer->SSAOBluredTexture);
	SetCombinedImageSampler(renderer, 6, renderer->shadowMap);
	SetCombinedImageSampler(renderer, 7, renderer->velocityGBuffer);
	SetCombinedImageSampler(renderer, 8, renderer->depthStencil);

	SetRenderTarget(renderer, 0, renderer->lightingMain);
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

	stateManager->SetVertexShader(renderer->GetShader("VS_PassThrough"));
	stateManager->SetPixelShader(renderer->GetShader("FS_PassThrough"));

	renderer->BindViewport(0, 0, GetUpscaledWidth, GetUpscaledHeight);

	SetCombinedImageSampler(renderer, 0, renderer->postUpscalePostEffects);

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
	SetRenderTarget(renderer, 0, renderer->lightingMain);

}

void OutlineEntityPass::Render(Renderer* renderer)
{
	renderer->DrawFullScreenQuad();
}

void SkyboxPass::DeclareResources(Renderer* renderer)
{

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
	SetCombinedImageSampler(renderer, 1, renderer->skyboxTexture);

	SetRenderTarget(renderer, 0, renderer->albedoGBuffer);
	SetDepthStencil(renderer, renderer->depthStencil);
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

		ImGui::SliderFloat("Radius: ", &renderer->ssaoParams.radius, 0.1f, 1.f);
		ImGui::SliderFloat("Bias:", &renderer->ssaoParams.bias, 0.0f, 0.0625f);
		ImGui::SliderFloat("Power:", &renderer->ssaoParams.power, 1.0f, 3.f);
		ImGui::SliderInt("Kernel Size: ", &renderer->ssaoParams.kernelSize, 1, 64);
		ImGui::Checkbox("SSSAO On: ", &renderer->ssaoParams.ssaoOn);
		ImGui::End();
	}

	renderer->SSAOParamsBuffer->FillBuffer(&renderer->ssaoParams, sizeof(SSAOParams));

	SetConstantBuffer(renderer, 0, renderer->GetMainConstBuffer());
	SetCombinedImageSampler(renderer, 1, renderer->worldPositionGBuffer);
	SetCombinedImageSampler(renderer, 2, renderer->normalGBuffer);
	SetCombinedImageSampler(renderer, 3, renderer->SSAONoiseTexture);
    SetConstantBuffer(renderer, 4, renderer->SSAOSamplesBuffer);
    SetConstantBuffer(renderer, 5, renderer->SSAOParamsBuffer);

	SetRenderTarget(renderer, 0, renderer->SSAOTexture);
}
void SSAOPass::Render(Renderer* renderer) 
{
	renderer->DrawFullScreenQuad();
}
void SSAOBlurPass::DeclareResources(Renderer* renderer)
{

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

	SetCombinedImageSampler(renderer, 0, renderer->SSAOTexture);
	SetRenderTarget(renderer, 0, renderer->SSAOBluredTexture);

}
void SSAOBlurPass::Render(Renderer* renderer)
{
	renderer->DrawFullScreenQuad();
}

void RTShadowsPass::DeclareResources(Renderer* renderer) 
{

}

void RTShadowsPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetComputeShader(renderer->GetShader("CS_RayTracingShadows"));

	SetStorageImage(renderer, 0, renderer->worldPositionGBuffer);
	SetStorageImage(renderer, 1, renderer->normalGBuffer);
	SetStorageImage(renderer, 2, renderer->shadowMap);
	SetStorageBuffer(renderer, 3, renderer->vertexBuffer);
	SetStorageBuffer(renderer, 4, renderer->trianglesBuffer);
	SetStorageBuffer(renderer, 5, renderer->triangleIndxsBuffer);
	SetStorageBuffer(renderer, 6, renderer->cfbvhNodesBuffer);
	SetConstantBuffer(renderer, 7, renderer->GetLightParams());
}

void RTShadowsPass::Render(Renderer* renderer)
{
	renderer->BindComputePipeline();

	renderer->BindDescriptorSets();

	static const int threadGroupWorkRegionDim = 8;
	int dispatchX = (GetNativeWidth + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
	int dispatchY = (GetNativeHeight + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;

	renderer->Dispatch(dispatchX, dispatchY, 1);
}

void FSR2Pass::DeclareResources(Renderer* renderer)
{

}

void FSR2Pass::Setup(Renderer* renderer)
{
	renderer->ResourceBarrier(renderer->fsrOutputTexture, ResourceState::GenericRead, ResourceState::GeneralCompute, ResourceStage::Graphics, ResourceStage::Compute);
	renderer->ResourceBarrier(renderer->volumetricOutput, ResourceState::RenderTarget, ResourceState::GenericRead, ResourceStage::Graphics, ResourceStage::Compute);
}

void FSR2Pass::Render(Renderer* renderer)
{
	SuperResolutionManager::FfxUpscaleSetup fsrSetup{};

	const auto& MainCamera = renderer->GetCamera();

	fsrSetup.cameraSetup.cameraPos = rabbitVec4f{ MainCamera->GetPosition(), 1.0f };
	fsrSetup.cameraSetup.cameraProj = MainCamera->ProjectionJittered();
	fsrSetup.cameraSetup.cameraView = MainCamera->View();
	fsrSetup.cameraSetup.cameraViewInv = glm::inverse(MainCamera->View());

	fsrSetup.depthbufferResource = renderer->depthStencil;
	fsrSetup.motionvectorResource = renderer->velocityGBuffer;
	fsrSetup.unresolvedColorResource = renderer->volumetricOutput;
	fsrSetup.resolvedColorResource = renderer->fsrOutputTexture;

	SuperResolutionManager::instance().Draw(renderer->GetCurrentCommandBuffer(), fsrSetup, renderer->GetUIState());

	renderer->ResourceBarrier(renderer->fsrOutputTexture, ResourceState::GeneralCompute, ResourceState::GenericRead, ResourceStage::Compute, ResourceStage::Graphics);
}

void VolumetricPass::DeclareResources(Renderer* renderer)
{}

void VolumetricPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetComputeShader(renderer->GetShader("CS_Volumetric"));

	SetStorageImage(renderer, 0, renderer->mediaDensity3DLUT);
	SetCombinedImageSampler(renderer, 1, renderer->noise3DLUT);
	SetConstantBuffer(renderer, 2, renderer->GetLightParams());
	SetConstantBuffer(renderer, 3, renderer->GetMainConstBuffer());
	SetConstantBuffer(renderer, 4, renderer->volumetricFogParamsBuffer);

	SetStorageBuffer(renderer, 5, renderer->vertexBuffer);
	SetStorageBuffer(renderer, 6, renderer->trianglesBuffer);
	SetStorageBuffer(renderer, 7, renderer->triangleIndxsBuffer);
	SetStorageBuffer(renderer, 8, renderer->cfbvhNodesBuffer);

	if (renderer->imguiReady)
	{
		ImGui::Begin("Volumetric Fog:");

		auto& fogParams = renderer->volumetricFogParams;
		
		static bool fogEnabled;
		ImGui::Checkbox("Enable Fog: ", &fogEnabled);
		fogParams.isEnabled = (uint32_t)fogEnabled;

		ImGui::SliderFloat("Fog Amount: ", &(fogParams.fogAmount), 0.0001f, 0.1f);
		ImGui::SliderFloat("Depth Scale Debug: ", &(fogParams.depthScale_debug), 0.1f, 5.f);
		ImGui::SliderFloat("Fog Start Distance ", &(fogParams.fogStartDistance), 0.01f, 20.f);
		ImGui::SliderFloat("Fog Distance ", &(fogParams.fogDistance), 10.f, 256.f);

		ImGui::End();
	}

	renderer->volumetricFogParamsBuffer->FillBuffer(&renderer->volumetricFogParams);
}

void VolumetricPass::Render(Renderer* renderer)
{
	renderer->BindComputePipeline();
	renderer->BindDescriptorSets();

	int texWidth = renderer->mediaDensity3DLUT->GetWidth();
	int texHeight = renderer->mediaDensity3DLUT->GetHeight();
	int texDepth = renderer->mediaDensity3DLUT->GetDepth();

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
	renderer->BindComputePipeline();
	renderer->BindDescriptorSets();

	renderer->Dispatch(256, 256, 256);
}


void ComputeScatteringPass::DeclareResources(Renderer* renderer)
{
}


void ComputeScatteringPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetComputeShader(renderer->GetShader("CS_ComputeScattering"));

	SetStorageImage(renderer, 0, renderer->mediaDensity3DLUT);
	SetStorageImage(renderer, 1, renderer->scatteringTexture);
}


void ComputeScatteringPass::Render(Renderer* renderer)
{
	renderer->BindComputePipeline();
	renderer->BindDescriptorSets();

	int texWidth = renderer->scatteringTexture->GetWidth();
	int texHeight = renderer->scatteringTexture->GetHeight();

	static const int threadGroupWorkRegionDim = 8;
	int dispatchX = (texWidth + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
	int dispatchY = (texHeight + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;

	renderer->Dispatch(dispatchX, dispatchY, 1);
}

void ApplyVolumetricFogPass::DeclareResources(Renderer* renderer)
{

}

void ApplyVolumetricFogPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetVertexShader(renderer->GetShader("VS_PassThrough"));
	stateManager->SetPixelShader(renderer->GetShader("FS_ApplyVolumetricFog"));

	SetCombinedImageSampler(renderer, 0, renderer->lightingMain);
	SetCombinedImageSampler(renderer, 1, renderer->depthStencil);
	SetCombinedImageSampler(renderer, 2, renderer->scatteringTexture);
	SetConstantBuffer(renderer, 3, renderer->GetMainConstBuffer());
	SetConstantBuffer(renderer, 4, renderer->volumetricFogParamsBuffer);

	SetRenderTarget(renderer, 0, renderer->volumetricOutput);
}

void ApplyVolumetricFogPass::Render(Renderer* renderer)
{
	renderer->DrawFullScreenQuad();
}

void TonemappingPass::DeclareResources(Renderer* renderer)
{

}

void TonemappingPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetVertexShader(renderer->GetShader("VS_PassThrough"));
	stateManager->SetPixelShader(renderer->GetShader("FS_Tonemap"));

	renderer->BindViewport(0, 0, GetUpscaledWidth, GetUpscaledHeight);

	SetCombinedImageSampler(renderer, 0, renderer->fsrOutputTexture);

	SetRenderTarget(renderer, 0, renderer->postUpscalePostEffects);
}

void TonemappingPass::Render(Renderer* renderer)
{
	renderer->DrawFullScreenQuad(true);
}

void TextureDebugPass::DeclareResources(Renderer* renderer)
{

}

void TextureDebugPass::Setup(Renderer* renderer)
{
    VulkanStateManager* stateManager = renderer->GetStateManager();

    stateManager->SetVertexShader(renderer->GetShader("VS_PassThrough"));
    stateManager->SetPixelShader(renderer->GetShader("FS_TextureDebug"));

	//see what texture is selected

    SetConstantBuffer(renderer, 0, renderer->debugTextureParamsBuffer);

	auto textureToBind = renderer->g_DefaultWhiteTexture;
	auto texture3DToBind = renderer->g_Default3DTexture;
	auto textureArrayToBind = renderer->g_DefaultArrayTexture;

	VulkanTexture* selectedTexture = renderer->GetTextureWithID(renderer->currentTextureSelectedID);

	if (selectedTexture)
	{
		auto& debugParams = renderer->debugTextureParams;
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

    renderer->debugTextureParamsBuffer->FillBuffer(&renderer->debugTextureParams);

    SetRenderTarget(renderer, 0, renderer->debugTexture);

}

void TextureDebugPass::Render(Renderer* renderer)
{
	renderer->DrawFullScreenQuad();
	
	renderer->ResourceBarrier(renderer->debugTexture, ResourceState::RenderTarget, ResourceState::GenericRead, ResourceStage::Graphics, ResourceStage::Graphics);
}