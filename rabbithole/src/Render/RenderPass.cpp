#include "RenderPass.h"
#include "Renderer.h"

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
	pipelineInfo->SetColorWriteMask(3, ColorWriteMaskFlags::R);
	
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
	stateManager->SetPixelShader(renderer->GetShader("FS_PhongBasicTest"));

	auto renderPassInfo = stateManager->GetRenderPassInfo();
	renderPassInfo->InitialRenderTargetState = ResourceState::None;
	renderPassInfo->FinalRenderTargetState = ResourceState::RenderTarget;
	renderPassInfo->InitialDepthStencilState = ResourceState::None;
	renderPassInfo->FinalDepthStencilState = ResourceState::DepthStencilWrite;

	constexpr size_t numOfLights = 4;
	LightParams lightParams[numOfLights];
	lightParams[0].position = { 7.0f, 1.0f, 7.0f, 0.0f };
	lightParams[0].colorAndRadius = { 0.0f, 0.0f, 1.0f, 5.0f };

	lightParams[1].position = { -7.0f, 1.0f, 7.0f, 0.0f };
	lightParams[1].colorAndRadius = { 1.0f, 0.0f, 0.0f, 5.0f };

	lightParams[2].position = { -10.0f, 2.0f, -10.0f, 0.0f };
	lightParams[2].colorAndRadius = { 1.0f, 0.6f, 0.2f, 10.0f };

	lightParams[3].position = { 0.0f, 25.0f, 0.0f, 0.0f };
	lightParams[3].colorAndRadius = { 1.0f, 1.0f, 0.0f, 30.0f };

	//fill the light buffer
	auto lightParamsBuffer = renderer->GetLightParams();
	void* data = lightParamsBuffer->Map();
	memcpy(data, &lightParams, sizeof(LightParams) * numOfLights);
	lightParamsBuffer->Unmap();

	auto& device = renderer->GetVulkanDevice();

	renderer->ResourceBarrier(renderer->albedoGBuffer, ResourceState::RenderTarget, ResourceState::GenericRead);
	renderer->ResourceBarrier(renderer->normalGBuffer, ResourceState::RenderTarget, ResourceState::GenericRead);
	renderer->ResourceBarrier(renderer->worldPositionGBuffer, ResourceState::RenderTarget, ResourceState::GenericRead);

	stateManager->SetCombinedImageSampler(0, renderer->albedoGBuffer);
	stateManager->SetCombinedImageSampler(1, renderer->normalGBuffer);
	stateManager->SetCombinedImageSampler(2, renderer->worldPositionGBuffer);
	stateManager->SetConstantBuffer(3, renderer->GetUniformBuffer(), 0, sizeof(UniformBufferObject));
	stateManager->SetConstantBuffer(4, renderer->GetLightParams(), 0, sizeof(LightParams));

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
	renderPassInfo->InitialRenderTargetState = ResourceState::GenericRead;
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

}

void OutlineEntityPass::Render(Renderer* renderer)
{
	renderer->DrawFullScreenQuad();
}


