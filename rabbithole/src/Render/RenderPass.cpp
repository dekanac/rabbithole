#include "RenderPass.h"
#include "Renderer.h"

float skyboxVertices[] = {
	// positions				//for now i got only one possible vertex binding and that is 3, 3, 3, 2 TODO: clean this
	-1000.0f,  1000.0f, -1000.0f,      0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
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

}

void GBufferPass::Setup(Renderer* renderer)
{
	VulkanStateManager* stateManager = renderer->GetStateManager();

	renderer->BindViewport(0, 0, Window::instance().GetExtent().width, Window::instance().GetExtent().height);
	stateManager->ShouldCleanColor(true);
	stateManager->ShouldCleanDepth(true);

	auto pipelineInfo = stateManager->GetPipelineInfo();
	
	pipelineInfo->SetAttachmentCount(4);
	pipelineInfo->SetColorWriteMask(0, ColorWriteMaskFlags::RGBA);
	pipelineInfo->SetColorWriteMask(1, ColorWriteMaskFlags::RGBA);
	pipelineInfo->SetColorWriteMask(2, ColorWriteMaskFlags::RGBA);
	pipelineInfo->SetColorWriteMask(3, ColorWriteMaskFlags::RGBA);
	
	stateManager->SetRenderTarget0(renderer->albedoGBuffer->GetView());
	stateManager->SetRenderTarget1(renderer->normalGBuffer->GetView());
	stateManager->SetRenderTarget2(renderer->worldPositionGBuffer->GetView());
	stateManager->SetRenderTarget3(renderer->entityHelper->GetView());
	stateManager->SetDepthStencil(renderer->GetSwapchain()->GetDepthStencil()->GetView());

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

		ImGui::ColorEdit3("Light2", lightParams[1].color);
		ImGui::SliderFloat("Light2 radius: ", &(lightParams[1]).radius, 0.f, 50.f);

		ImGui::ColorEdit3("Light3", lightParams[2].color);
		ImGui::SliderFloat("Light3 radius: ", &(lightParams[2]).radius, 0.f, 50.f);

		ImGui::ColorEdit3("Light4", lightParams[3].color);
		ImGui::SliderFloat("Light4 radius: ", &(lightParams[3]).radius, 0.f, 50.f);

		ImGui::End();
	}
	renderer->GetLightParams()->FillBuffer(renderer->lightParams, sizeof(LightParams) * numOfLights);

	auto& device = renderer->GetVulkanDevice();

	renderer->ResourceBarrier(renderer->albedoGBuffer, ResourceState::RenderTarget, ResourceState::GenericRead);
	//renderer->ResourceBarrier(renderer->normalGBuffer, ResourceState::RenderTarget, ResourceState::GenericRead);
	renderer->ResourceBarrier(renderer->worldPositionGBuffer, ResourceState::RenderTarget, ResourceState::GenericRead);
	renderer->ResourceBarrier(renderer->SSAOBluredTexture, ResourceState::RenderTarget, ResourceState::GenericRead);

	stateManager->SetCombinedImageSampler(0, renderer->albedoGBuffer);
	stateManager->SetCombinedImageSampler(1, renderer->normalGBuffer);
	stateManager->SetCombinedImageSampler(2, renderer->worldPositionGBuffer);
	stateManager->SetConstantBuffer(3, renderer->GetUniformBuffer(), 0, sizeof(UniformBufferObject));
	stateManager->SetConstantBuffer(4, renderer->GetLightParams(), 0, sizeof(LightParams));
	stateManager->SetCombinedImageSampler(5, renderer->SSAOBluredTexture);

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

	stateManager->SetVertexShader(renderer->GetShader("VS_PassThrough"));
	stateManager->SetPixelShader(renderer->GetShader("FS_PassThrough"));

	renderer->ResourceBarrier(renderer->lightingMain, ResourceState::RenderTarget, ResourceState::GenericRead);
	stateManager->SetCombinedImageSampler(0, renderer->lightingMain);

	stateManager->SetRenderTarget0(renderer->GetSwapchainImage());

	stateManager->GetPipelineInfo()->SetDepthTestEnabled(false);
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

	stateManager->SetConstantBuffer(0, renderer->GetUniformBuffer(), 0, sizeof(UniformBufferObject));
	stateManager->SetCombinedImageSampler(1, renderer->skyboxTexture);

	stateManager->SetRenderTarget0(renderer->albedoGBuffer->GetView());
	stateManager->SetDepthStencil(renderer->GetSwapchain()->GetDepthStencil()->GetView());
}
void SkyboxPass::Render(Renderer* renderer)
{
	//TODO: dont do like this, make one big DEVICE LOCAL buffer and calculate offsets
	renderer->GetVertexUploadBuffer()->FillBuffer(skyboxVertices, 396 * sizeof(float));
	renderer->BindVertexData();
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
		ImGui::End();
	}

	renderer->SSAOParamsBuffer->FillBuffer(&renderer->ssaoParams, sizeof(SSAOParams));

	renderer->ResourceBarrier(renderer->GetSwapchain()->GetDepthStencil(), ResourceState::DepthStencilWrite, ResourceState::GenericRead);
	renderer->ResourceBarrier(renderer->normalGBuffer, ResourceState::RenderTarget, ResourceState::GenericRead);

	stateManager->SetConstantBuffer(0, renderer->GetUniformBuffer(), 0, sizeof(UniformBufferObject));
	stateManager->SetCombinedImageSampler(1, renderer->GetSwapchain()->GetDepthStencil());
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
	/////////// Test for COMPUTE SHADER
	stateManager->SetComputeShader(renderer->GetShader("CS_Example"));

	renderer->BindComputePipeline();

	renderer->ResourceBarrier(renderer->albedoGBuffer, ResourceState::RenderTarget, ResourceState::GeneralCompute);

	stateManager->SetStorageImage(0, renderer->albedoGBuffer);
	stateManager->SetStorageImage(1, renderer->albedoGBuffer);

	renderer->BindDescriptorSets();

	renderer->Dispatch(1280 / 16, 720 / 16, 1);

	renderer->ResourceBarrier(renderer->albedoGBuffer, ResourceState::GeneralCompute, ResourceState::RenderTarget);
	///////////
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


