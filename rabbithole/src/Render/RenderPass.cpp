#include "RenderPass.h"
#include "Renderer.h"

#include "SuperResolutionManager.h"

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

	stateManager->SetConstantBuffer(0, renderer->GetMainConstBuffer(), 0, sizeof(UniformBufferObject));

	stateManager->SetRenderTarget0(renderer->lightingMain->GetView());
	stateManager->SetDepthStencil(renderer->depthStencil->GetView());

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

	stateManager->SetRenderTarget0(renderer->albedoGBuffer->GetView());
	stateManager->SetRenderTarget1(renderer->normalGBuffer->GetView());
	stateManager->SetRenderTarget2(renderer->worldPositionGBuffer->GetView());
	stateManager->SetRenderTarget3(renderer->entityHelper->GetView());
	stateManager->SetRenderTarget4(renderer->velocityGBuffer->GetView());
	stateManager->SetDepthStencil(renderer->depthStencil->GetView());

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

	auto renderPassInfo = stateManager->GetRenderPassInfo();
	renderPassInfo->InitialRenderTargetState = ResourceState::None;
	renderPassInfo->FinalRenderTargetState = ResourceState::RenderTarget;
	renderPassInfo->InitialDepthStencilState = ResourceState::None;
	renderPassInfo->FinalDepthStencilState = ResourceState::DepthStencilWrite;

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

	renderer->ResourceBarrier(renderer->albedoGBuffer, ResourceState::RenderTarget, ResourceState::GenericRead);
	renderer->ResourceBarrier(renderer->SSAOBluredTexture, ResourceState::RenderTarget, ResourceState::GenericRead);
	renderer->ResourceBarrier(renderer->velocityGBuffer, ResourceState::RenderTarget, ResourceState::GenericRead);

	stateManager->SetCombinedImageSampler(0, renderer->albedoGBuffer);
	stateManager->SetCombinedImageSampler(1, renderer->normalGBuffer);
	stateManager->SetCombinedImageSampler(2, renderer->worldPositionGBuffer);
	stateManager->SetConstantBuffer(3, renderer->GetMainConstBuffer(), 0, sizeof(UniformBufferObject));
	stateManager->SetConstantBuffer(4, renderer->GetLightParams(), 0, sizeof(LightParams));
	stateManager->SetCombinedImageSampler(5, renderer->SSAOBluredTexture);
	stateManager->SetCombinedImageSampler(6, renderer->shadowMap);
	stateManager->SetCombinedImageSampler(7, renderer->velocityGBuffer);

	stateManager->SetRenderTarget0(renderer->lightingMain->GetView());
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

	stateManager->SetCombinedImageSampler(0, renderer->fsrOutputTexture);

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

	stateManager->SetCombinedImageSampler(0, renderer->entityHelper);
	stateManager->SetRenderTarget0(renderer->lightingMain->GetView());

	renderer->ResourceBarrier(renderer->entityHelper, ResourceState::RenderTarget, ResourceState::GenericRead);

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
	
	stateManager->ShouldCleanColor(false);
	stateManager->ShouldCleanDepth(false);

	auto pipelineInfo = stateManager->GetPipelineInfo();
	pipelineInfo->SetDepthTestEnabled(true);

	auto renderPassInfo = stateManager->GetRenderPassInfo();
	renderPassInfo->InitialRenderTargetState = ResourceState::RenderTarget;
	renderPassInfo->FinalRenderTargetState = ResourceState::RenderTarget;
	renderPassInfo->InitialDepthStencilState = ResourceState::GenericRead;
	renderPassInfo->FinalDepthStencilState = ResourceState::DepthStencilWrite;

	stateManager->SetCullMode(CullMode::Front);

	stateManager->SetVertexShader(renderer->GetShader("VS_Skybox"));
	stateManager->SetPixelShader(renderer->GetShader("FS_Skybox"));

	stateManager->SetConstantBuffer(0, renderer->GetMainConstBuffer(), 0, sizeof(UniformBufferObject));
	stateManager->SetCombinedImageSampler(1, renderer->skyboxTexture);

	stateManager->SetRenderTarget0(renderer->albedoGBuffer->GetView());
	stateManager->SetDepthStencil(renderer->depthStencil->GetView());
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

	renderer->ResourceBarrier(renderer->depthStencil, ResourceState::DepthStencilWrite, ResourceState::GenericRead);
	renderer->ResourceBarrier(renderer->normalGBuffer, ResourceState::RenderTarget, ResourceState::GenericRead);

	stateManager->SetConstantBuffer(0, renderer->GetMainConstBuffer(), 0, sizeof(UniformBufferObject));
	stateManager->SetCombinedImageSampler(1, renderer->depthStencil);
	stateManager->SetCombinedImageSampler(2, renderer->normalGBuffer);
	stateManager->SetCombinedImageSampler(3, renderer->SSAONoiseTexture);
	stateManager->SetConstantBuffer(4, renderer->SSAOSamplesBuffer, 0, renderer->SSAOSamplesBuffer->GetSize());
	stateManager->SetConstantBuffer(5, renderer->SSAOParamsBuffer, 0, renderer->SSAOParamsBuffer->GetSize());

	stateManager->SetRenderTarget0(renderer->SSAOTexture->GetView());
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

	renderer->ResourceBarrier(renderer->SSAOTexture, ResourceState::RenderTarget, ResourceState::GenericRead);

	stateManager->SetCombinedImageSampler(0, renderer->SSAOTexture);
	stateManager->SetRenderTarget0(renderer->SSAOBluredTexture->GetView());

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

	renderer->ResourceBarrier(renderer->shadowMap, ResourceState::GenericRead, ResourceState::GeneralCompute);
	renderer->ResourceBarrier(renderer->worldPositionGBuffer, ResourceState::RenderTarget, ResourceState::GeneralCompute);
	renderer->ResourceBarrier(renderer->normalGBuffer, ResourceState::GenericRead, ResourceState::GeneralCompute);

	stateManager->SetStorageImage(0, renderer->worldPositionGBuffer);
	stateManager->SetStorageImage(1, renderer->normalGBuffer);
	stateManager->SetStorageImage(2, renderer->shadowMap);
	stateManager->SetStorageBuffer(3, renderer->vertexBuffer, 0, renderer->vertexBuffer->GetSize());
	stateManager->SetStorageBuffer(4, renderer->trianglesBuffer, 0, renderer->trianglesBuffer->GetSize());
	stateManager->SetStorageBuffer(5, renderer->triangleIndxsBuffer, 0, renderer->triangleIndxsBuffer->GetSize());
	stateManager->SetStorageBuffer(6, renderer->cfbvhNodesBuffer, 0, renderer->cfbvhNodesBuffer->GetSize());
	stateManager->SetConstantBuffer(7, renderer->GetLightParams(), 0, sizeof(LightParams));
}

void RTShadowsPass::Render(Renderer* renderer)
{
	renderer->BindComputePipeline();

	renderer->BindDescriptorSets();

	static const int threadGroupWorkRegionDim = 16;
	int dispatchX = (GetNativeWidth + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
	int dispatchY = (GetNativeHeight + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
	renderer->Dispatch(dispatchX, dispatchY, 1);

	renderer->ResourceBarrier(renderer->shadowMap, ResourceState::GeneralCompute, ResourceState::GenericRead);
	renderer->ResourceBarrier(renderer->worldPositionGBuffer, ResourceState::GeneralCompute, ResourceState::GenericRead);
	renderer->ResourceBarrier(renderer->normalGBuffer, ResourceState::GeneralCompute, ResourceState::GenericRead);
}

void FSRPass::DeclareResources(Renderer* renderer)
{}

void FSRPass::Setup(Renderer* renderer)
{

}

void FSRPass::Render(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	//EASU pass
	stateManager->SetComputeShader(renderer->GetShader("CS_FSR_EASU"));

	renderer->ResourceBarrier(renderer->lightingMain, ResourceState::GeneralCompute, ResourceState::GenericRead);
	renderer->ResourceBarrier(renderer->fsrIntermediateRes, ResourceState::GenericRead, ResourceState::GeneralCompute);

	stateManager->SetConstantBuffer(0, renderer->fsrParamsBuffer, 0, renderer->fsrParamsBuffer->GetSize());
	stateManager->SetSampledImage(1, renderer->lightingMain);
	stateManager->SetStorageImage(2, renderer->fsrIntermediateRes);
	stateManager->SetSampler(3, renderer->lightingMain);

	renderer->BindComputePipeline();
	renderer->BindDescriptorSets();

	static const int threadGroupWorkRegionDim = 16;
	int dispatchX = (GetUpscaledWidth + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
	int dispatchY = (GetUpscaledHeight + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;

	renderer->Dispatch(dispatchX, dispatchY, 1);

	//RCAS pass
	stateManager->SetComputeShader(renderer->GetShader("CS_FSR_RCAS"));

	renderer->ResourceBarrier(renderer->fsrIntermediateRes, ResourceState::GeneralCompute, ResourceState::GenericRead);
	renderer->ResourceBarrier(renderer->fsrOutputTexture, ResourceState::GenericRead, ResourceState::GeneralCompute);

	stateManager->SetConstantBuffer(0, renderer->fsrParamsBuffer, 0, renderer->fsrParamsBuffer->GetSize());
	stateManager->SetSampledImage(1, renderer->fsrIntermediateRes);
	stateManager->SetStorageImage(2, renderer->fsrOutputTexture);
	stateManager->SetSampler(3, renderer->fsrIntermediateRes);

	renderer->BindComputePipeline();
	renderer->BindDescriptorSets();

	renderer->Dispatch(dispatchX, dispatchY, 1);

	renderer->ResourceBarrier(renderer->fsrOutputTexture, ResourceState::GeneralCompute, ResourceState::GenericRead);

}

void TAAPass::DeclareResources(Renderer* renderer)
{

}

void TAAPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	stateManager->SetComputeShader(renderer->GetShader("CS_TAA"));

	stateManager->SetSampledImage(0, renderer->lightingMain);
	stateManager->SetSampledImage(1, renderer->depthStencil);
	stateManager->SetSampledImage(2, renderer->historyBuffer);
	stateManager->SetSampledImage(3, renderer->velocityGBuffer);

	stateManager->SetStorageImage(4, renderer->TAAOutput);

	stateManager->SetSampler(5, renderer->lightingMain);
	stateManager->SetSampler(6, renderer->depthStencil);
	stateManager->SetSampler(7, renderer->historyBuffer);
	stateManager->SetSampler(8, renderer->velocityGBuffer);

	renderer->ResourceBarrier(renderer->TAAOutput, ResourceState::GenericRead, ResourceState::GeneralCompute);
	renderer->ResourceBarrier(renderer->lightingMain, ResourceState::RenderTarget, ResourceState::GenericRead);
	renderer->ResourceBarrier(renderer->depthStencil, ResourceState::DepthStencilWrite, ResourceState::GenericRead);
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

	stateManager->SetSampledImage(0, renderer->TAAOutput);

	stateManager->SetStorageImage(1, renderer->lightingMain);
	stateManager->SetStorageImage(2, renderer->historyBuffer);

	renderer->ResourceBarrier(renderer->TAAOutput, ResourceState::GeneralCompute, ResourceState::GenericRead);
	renderer->ResourceBarrier(renderer->lightingMain, ResourceState::GenericRead, ResourceState::GeneralCompute);
	renderer->ResourceBarrier(renderer->historyBuffer, ResourceState::GenericRead, ResourceState::GeneralCompute);
}

void TAASharpenerPass::Render(Renderer* renderer) 
{
	renderer->BindComputePipeline();
	renderer->BindDescriptorSets();

	static const int threadGroupWorkRegionDim = 8;
	int dispatchX = (GetNativeWidth + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
	int dispatchY = (GetNativeHeight + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;

	renderer->Dispatch(dispatchX, dispatchY, 1);

	renderer->ResourceBarrier(renderer->historyBuffer, ResourceState::GeneralCompute, ResourceState::GenericRead);
}

