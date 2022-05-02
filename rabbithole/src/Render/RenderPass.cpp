#include "RenderPass.h"
#include "Renderer.h"
#include "ResourceStateTracking.h"

#include "SuperResolutionManager.h"

void RenderPass::SetCombinedImageSampler(Renderer* renderer, int slot, VulkanTexture* texture)
{
	texture->SetShouldBeResourceState(ResourceState::GenericRead);
	RSTManager.AddResourceForTransition(texture);
	renderer->GetStateManager()->SetCombinedImageSampler(slot, texture);
}

void RenderPass::SetSampledImage(Renderer* renderer, int slot, VulkanTexture* texture)
{
	texture->SetShouldBeResourceState(ResourceState::GenericRead);
	RSTManager.AddResourceForTransition(texture);
	renderer->GetStateManager()->SetSampledImage(slot, texture);
}

void RenderPass::SetStorageImage(Renderer* renderer, int slot, VulkanTexture* texture)
{
	texture->SetShouldBeResourceState(ResourceState::GeneralCompute);
	RSTManager.AddResourceForTransition(texture);
	renderer->GetStateManager()->SetStorageImage(slot, texture);
}

void RenderPass::SetRenderTarget(Renderer* renderer, int slot, VulkanTexture* texture)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

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
	//TODO: missing transition handling
	VulkanStateManager* stateManager = renderer->GetStateManager();
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

	renderer->BindViewport(0, 0, GetNativeWidth, GetNativeHeight);
	stateManager->ShouldCleanColor(false);
	stateManager->ShouldCleanDepth(false);

	auto pipelineInfo = stateManager->GetPipelineInfo();
	pipelineInfo->SetTopology(Topology::LineList);
	pipelineInfo->SetCullMode(CullMode::None);

	stateManager->SetConstantBuffer(0, renderer->GetMainConstBuffer());

	SetRenderTarget(renderer, 0, renderer->lightingMain);
	SetDepthStencil(renderer, renderer->depthStencil);

	auto renderPassInfo = stateManager->GetRenderPassInfo();

	stateManager->GetPipelineInfo()->SetDepthTestEnabled(true);
	renderPassInfo->InitialRenderTargetState = ResourceState::RenderTarget;
	renderPassInfo->FinalRenderTargetState = ResourceState::RenderTarget;
	renderPassInfo->InitialDepthStencilState = ResourceState::DepthStencilWrite;
	renderPassInfo->FinalDepthStencilState = ResourceState::DepthStencilWrite;

	stateManager->SetVertexShader(renderer->GetShader("VS_SimpleGeometry"));
	stateManager->SetPixelShader(renderer->GetShader("FS_SimpleGeometry"));

}

void BoundingBoxPass::Render(Renderer* renderer)
{
	renderer->DrawBoundingBoxes(renderer->GetModels());
}

void GBufferPass::DeclareResources(Renderer* renderer)
{

}

void GBufferPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	renderer->BindViewport(0, 0, GetNativeWidth, GetNativeHeight);
	stateManager->ShouldCleanColor(true);
	stateManager->ShouldCleanDepth(true);

	auto pipelineInfo = stateManager->GetPipelineInfo();

	pipelineInfo->SetAttachmentCount(5);
	pipelineInfo->SetColorWriteMask(0, ColorWriteMaskFlags::RGBA);
	pipelineInfo->SetColorWriteMask(1, ColorWriteMaskFlags::RGBA);
	pipelineInfo->SetColorWriteMask(2, ColorWriteMaskFlags::RGBA);
	pipelineInfo->SetColorWriteMask(3, ColorWriteMaskFlags::RGBA);
	pipelineInfo->SetColorWriteMask(4, ColorWriteMaskFlags::RGBA);

	SetRenderTarget(renderer, 0, renderer->albedoGBuffer);
	SetRenderTarget(renderer, 1, renderer->normalGBuffer);
	SetRenderTarget(renderer, 2, renderer->worldPositionGBuffer);
	SetRenderTarget(renderer, 3, renderer->entityHelper);
	SetRenderTarget(renderer, 4, renderer->velocityGBuffer);
	SetDepthStencil(renderer, renderer->depthStencil);

	auto renderPassInfo = stateManager->GetRenderPassInfo();

	stateManager->GetPipelineInfo()->SetDepthTestEnabled(true);
	renderPassInfo->InitialRenderTargetState = ResourceState::None;
	renderPassInfo->FinalRenderTargetState = ResourceState::RenderTarget;
	renderPassInfo->InitialDepthStencilState = ResourceState::None;
	renderPassInfo->FinalDepthStencilState = ResourceState::DepthStencilWrite;

	stateManager->SetCullMode(CullMode::Front);//TODO: VULKAN INVERT Y FLIP FIX
	stateManager->SetVertexShader(renderer->GetShader("VS_GBuffer"));
	stateManager->SetPixelShader(renderer->GetShader("FS_GBuffer"));

	renderer->BindCameraMatrices(renderer->GetCamera());
}

void GBufferPass::Render(Renderer* renderer)
{
	renderer->DrawGeometry(renderer->GetModels());
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

		ImGui::ColorEdit3("Light1", lightParams[0].color);
		ImGui::SliderFloat("Light1 radius: ", &(lightParams[0]).radius, 0.f, 50.f);
		ImGui::SliderFloat3("Light1 position: ", lightParams[0].position, -10.f, 10.f);

		ImGui::ColorEdit3("Light2", lightParams[1].color);
		ImGui::SliderFloat("Light2 radius: ", &(lightParams[1]).radius, 0.f, 50.f);
		ImGui::SliderFloat3("Light2 position: ", lightParams[1].position, -10.f, 10.f);

		ImGui::ColorEdit3("Light3", lightParams[2].color);
		ImGui::SliderFloat("Light3 radius: ", &(lightParams[2]).radius, 0.f, 50.f);
		ImGui::SliderFloat3("Light3 position: ", lightParams[2].position, -10.f, 10.f);

		ImGui::ColorEdit3("Light4", lightParams[3].color);
		ImGui::SliderFloat("Light4 radius: ", &(lightParams[3]).radius, 0.f, 50.f);
		ImGui::SliderFloat3("Light4 position: ", lightParams[3].position, -10.f, 10.f);

		ImGui::End();
	}
	renderer->GetLightParams()->FillBuffer(renderer->lightParams, sizeof(LightParams) * numOfLights);

	auto& device = renderer->GetVulkanDevice();

	SetCombinedImageSampler(renderer, 0, renderer->albedoGBuffer);
	SetCombinedImageSampler(renderer, 1, renderer->normalGBuffer);
	SetCombinedImageSampler(renderer, 2, renderer->worldPositionGBuffer);
	stateManager->SetConstantBuffer(3, renderer->GetMainConstBuffer());
	stateManager->SetConstantBuffer(4, renderer->GetLightParams());
	SetCombinedImageSampler(renderer, 5, renderer->SSAOBluredTexture);
	SetCombinedImageSampler(renderer, 6, renderer->shadowMap);
	SetCombinedImageSampler(renderer, 7, renderer->velocityGBuffer);

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

	renderer->BindViewport(0, 0, GetUpscaledWidth, GetUpscaledHeight);
	stateManager->SetVertexShader(renderer->GetShader("VS_PassThrough"));
	stateManager->SetPixelShader(renderer->GetShader("FS_PassThrough"));

	SetCombinedImageSampler(renderer, 0, renderer->fsrOutputTexture);

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

	stateManager->ShouldCleanColor(false);

	renderer->UpdateEntityPickId();

	auto pipelineInfo = stateManager->GetPipelineInfo();
	pipelineInfo->SetAttachmentCount(1);
	pipelineInfo->SetColorWriteMask(0, ColorWriteMaskFlags::RGBA);

	auto renderPassInfo = stateManager->GetRenderPassInfo();
	renderPassInfo->InitialRenderTargetState = ResourceState::None;
	renderPassInfo->FinalRenderTargetState = ResourceState::RenderTarget;

	stateManager->SetVertexShader(renderer->GetShader("VS_PassThrough"));
	stateManager->SetPixelShader(renderer->GetShader("FS_OutlineEntity"));

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
	
	stateManager->ShouldCleanDepth(false);

	auto pipelineInfo = stateManager->GetPipelineInfo();
	pipelineInfo->SetDepthTestEnabled(true);

	auto renderPassInfo = stateManager->GetRenderPassInfo();

	stateManager->SetCullMode(CullMode::Front);

	stateManager->SetVertexShader(renderer->GetShader("VS_Skybox"));
	stateManager->SetPixelShader(renderer->GetShader("FS_Skybox"));

	stateManager->SetConstantBuffer(0, renderer->GetMainConstBuffer());
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
		ImGui::SliderInt("Kernel Size: ", &renderer->ssaoParams.kernelSize, 1, 64);
		ImGui::Checkbox("SSSAO On: ", &renderer->ssaoParams.ssaoOn);
		ImGui::End();
	}

	renderer->SSAOParamsBuffer->FillBuffer(&renderer->ssaoParams, sizeof(SSAOParams));

	stateManager->SetConstantBuffer(0, renderer->GetMainConstBuffer());
	SetCombinedImageSampler(renderer, 1, renderer->depthStencil);
	SetCombinedImageSampler(renderer, 2, renderer->normalGBuffer);
	SetCombinedImageSampler(renderer, 3, renderer->SSAONoiseTexture);
	stateManager->SetConstantBuffer(4, renderer->SSAOSamplesBuffer);
	stateManager->SetConstantBuffer(5, renderer->SSAOParamsBuffer);

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
	stateManager->SetStorageBuffer(3, renderer->vertexBuffer);
	stateManager->SetStorageBuffer(4, renderer->trianglesBuffer);
	stateManager->SetStorageBuffer(5, renderer->triangleIndxsBuffer);
	stateManager->SetStorageBuffer(6, renderer->cfbvhNodesBuffer);
	stateManager->SetConstantBuffer(7, renderer->GetLightParams());
}

void RTShadowsPass::Render(Renderer* renderer)
{
	renderer->BindComputePipeline();

	renderer->BindDescriptorSets();

	static const int threadGroupWorkRegionDim = 16;
	int dispatchX = (GetNativeWidth + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
	int dispatchY = (GetNativeHeight + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
	renderer->Dispatch(dispatchX, dispatchY, 1);

}

void FSREASUPass::DeclareResources(Renderer* renderer)
{}

void FSREASUPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetComputeShader(renderer->GetShader("CS_FSR_EASU"));

	stateManager->SetConstantBuffer(0, renderer->fsrParamsBuffer);
	SetSampledImage(renderer, 1, renderer->lightingMain);
	SetStorageImage(renderer, 2, renderer->fsrIntermediateRes);
	stateManager->SetSampler(3, renderer->lightingMain);
}

void FSREASUPass::Render(Renderer* renderer)
{
	renderer->BindComputePipeline();
	renderer->BindDescriptorSets();

	static const int threadGroupWorkRegionDim = 16;
	int dispatchX = (GetUpscaledWidth + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
	int dispatchY = (GetUpscaledHeight + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;

	renderer->Dispatch(dispatchX, dispatchY, 1);
}

void FSRRCASPass::DeclareResources(Renderer* renderer)
{}

void FSRRCASPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetComputeShader(renderer->GetShader("CS_FSR_RCAS"));

	stateManager->SetConstantBuffer(0, renderer->fsrParamsBuffer);
	SetSampledImage(renderer, 1, renderer->fsrIntermediateRes);
	SetStorageImage(renderer, 2, renderer->fsrOutputTexture);
	stateManager->SetSampler(3, renderer->fsrIntermediateRes);
}

void FSRRCASPass::Render(Renderer* renderer)
{
	renderer->BindComputePipeline();
	renderer->BindDescriptorSets();

	static const int threadGroupWorkRegionDim = 16;
	int dispatchX = (GetUpscaledWidth + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
	int dispatchY = (GetUpscaledHeight + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;

	renderer->Dispatch(dispatchX, dispatchY, 1);
}

void TAAPass::DeclareResources(Renderer* renderer)
{

}

void TAAPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetComputeShader(renderer->GetShader("CS_TAA"));

	SetSampledImage(renderer, 0, renderer->lightingMain);
	SetSampledImage(renderer, 1, renderer->depthStencil);
	SetSampledImage(renderer, 2, renderer->historyBuffer);
	SetSampledImage(renderer, 3, renderer->velocityGBuffer);

	SetStorageImage(renderer, 4, renderer->TAAOutput);

	stateManager->SetSampler(5, renderer->lightingMain);
	stateManager->SetSampler(6, renderer->depthStencil);
	stateManager->SetSampler(7, renderer->historyBuffer);
	stateManager->SetSampler(8, renderer->velocityGBuffer);
}

void TAAPass::Render(Renderer* renderer)
{
	renderer->BindComputePipeline();
	renderer->BindDescriptorSets();

	static const int threadGroupWorkRegionDim = 16;
	int dispatchX = (GetNativeWidth + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
	int dispatchY = (GetNativeHeight + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;

	renderer->Dispatch(dispatchX, dispatchY, 1);

}

void TAASharpenerPass::DeclareResources(Renderer* renderer) 
{}

void TAASharpenerPass::Setup(Renderer* renderer) 
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetComputeShader(renderer->GetShader("CS_TAASharpener"));

	SetSampledImage(renderer, 0, renderer->TAAOutput);

	SetStorageImage(renderer, 1, renderer->lightingMain);
	SetStorageImage(renderer, 2, renderer->historyBuffer);
}

void TAASharpenerPass::Render(Renderer* renderer) 
{
	renderer->BindComputePipeline();
	renderer->BindDescriptorSets();

	static const int threadGroupWorkRegionDim = 8;
	int dispatchX = (GetNativeWidth + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
	int dispatchY = (GetNativeHeight + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;

	renderer->Dispatch(dispatchX, dispatchY, 1);
}
