#include "GBuffer.h"

defineResource(GBufferPass, Albedo, VulkanTexture);
defineResource(GBufferPass, Normals, VulkanTexture);
defineResource(GBufferPass, Velocity, VulkanTexture);
defineResource(GBufferPass, Emissive, VulkanTexture);
defineResource(GBufferPass, WorldPosition, VulkanTexture);
defineResource(GBufferPass, Depth, VulkanTexture);

defineResource(SkyboxPass, Main, VulkanTexture);

defineResource(CopyDepthPass, DepthR32, VulkanTexture);

void GBufferPass::DeclareResources()
{
	Albedo = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth , GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read},
			.format = {Format::R8G8B8A8_UNORM},
			.name = {"GBuffer Albedo"}
		});

	Emissive = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth , GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read},
			.format = {Format::R8G8B8A8_UNORM},
			.name = {"GBuffer Emissive"},
			.arraySize = {1},
			.isCube = false,
			.multisampleType = MultisampleType::Sample_1,
			.samplerType = SamplerType::Bilinear,
			.addressMode = AddressMode::Repeat,
			.mipCount = 1,
			.clearValue = BLACK_COLOR
		});

	Normals = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth , GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::Storage},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"GBuffer Normal"}
		});

	WorldPosition = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth , GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::Storage},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"GBuffer World Position"}
		});

	Velocity = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth , GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::Storage},
			.format = {Format::R16G16_FLOAT},
			.name = {"GBuffer Velocity"},
			.samplerType = SamplerType::Point
		});

	Depth = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth , GetNativeHeight, 1},
			.flags = {TextureFlags::DepthStencil | TextureFlags::Read | TextureFlags::TransferSrc},
			.format = {Format::D32_SFLOAT},
			.name = {"GBuffer Depth"},
			.samplerType = SamplerType::Point
		});
}

void GBufferPass::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	stateManager.SetVertexShader(m_Renderer.GetShader("VS_GBuffer"));
	stateManager.SetPixelShader(m_Renderer.GetShader("FS_GBuffer"));

	m_Renderer.BindViewport(0, 0, static_cast<float>(GetNativeWidth), static_cast<float>(GetNativeHeight));
	stateManager.SetRenderPassExtent({ GetNativeWidth , GetNativeHeight });
	stateManager.ShouldCleanColor(LoadOp::Clear);
	stateManager.ShouldCleanDepth(LoadOp::Clear);

	auto pipelineInfo = stateManager.GetPipelineInfo();
	pipelineInfo->SetDepthWriteEnabled(true);
	pipelineInfo->SetAttachmentCount(5);
	pipelineInfo->SetColorWriteMask(0, ColorWriteMaskFlags::RGBA);
	pipelineInfo->SetColorWriteMask(1, ColorWriteMaskFlags::RGBA);
	pipelineInfo->SetColorWriteMask(2, ColorWriteMaskFlags::RGBA);
	pipelineInfo->SetColorWriteMask(3, ColorWriteMaskFlags::RGBA);
	pipelineInfo->SetColorWriteMask(4, ColorWriteMaskFlags::RGBA);

	SetConstantBuffer(0, m_Renderer.GetMainConstBuffer());

	SetRenderTarget(0, GBufferPass::Albedo);
	SetRenderTarget(1, GBufferPass::Normals);
	SetRenderTarget(2, GBufferPass::WorldPosition);
	SetRenderTarget(3, GBufferPass::Velocity);
	SetRenderTarget(4, GBufferPass::Emissive);
	SetDepthStencil(GBufferPass::Depth);

	auto renderPassInfo = stateManager.GetRenderPassInfo();

	stateManager.GetPipelineInfo()->SetDepthTestEnabled(true);
	renderPassInfo->InitialRenderTargetState =	ResourceState::RenderTarget;
	renderPassInfo->FinalRenderTargetState =	ResourceState::RenderTarget;
	renderPassInfo->InitialDepthStencilState =	ResourceState::DepthStencilWrite;
	renderPassInfo->FinalDepthStencilState =	ResourceState::DepthStencilWrite;

	stateManager.SetCullMode(CullMode::Front);
}

void GBufferPass::Render()
{
	m_Renderer.DrawGeometryGLTF(m_Renderer.gltfModels);
}

void CopyDepthPass::DeclareResources()
{
	DepthR32 = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), RWTextureCreateInfo{
			.dimensions = {GetNativeWidth , GetNativeHeight, 1},
			.flags = {TextureFlags::Read | TextureFlags::Storage | TextureFlags::RenderTarget | TextureFlags::TransferSrc},
			.format = {Format::R32_SFLOAT},
			.name = {"DepthR32"}
		});
}

void CopyDepthPass::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	stateManager.SetVertexShader(m_Renderer.GetShader("VS_PassThrough"));
	stateManager.SetPixelShader(m_Renderer.GetShader("FS_CopyDepth"));

	auto renderPassInfo = stateManager.GetRenderPassInfo();
	renderPassInfo->InitialRenderTargetState = ResourceState::RenderTarget;
	renderPassInfo->FinalRenderTargetState = ResourceState::RenderTarget;

	SetCombinedImageSampler(0, GBufferPass::Depth);

	SetRenderTarget(0, CopyDepthPass::DepthR32);
}

void CopyDepthPass::Render()
{
	m_Renderer.DrawFullScreenQuad();
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

void SkyboxPass::DeclareResources()
{
	Main = m_Renderer.GetResourceManager().CreateTexture(m_Renderer.GetVulkanDevice(), "res/textures/skybox/skybox.jpg", ROTextureCreateInfo{
			.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::CubeMap | TextureFlags::TransferDst},
			.format = {Format::R8G8B8A8_UNORM_SRGB},
			.name = {"Skybox"}
		});
}

void SkyboxPass::Setup()
{
	VulkanStateManager& stateManager = m_Renderer.GetStateManager();

	stateManager.SetVertexShader(m_Renderer.GetShader("VS_Skybox"));
	stateManager.SetPixelShader(m_Renderer.GetShader("FS_Skybox"));

	stateManager.ShouldCleanDepth(LoadOp::Load);
	stateManager.ShouldCleanColor(LoadOp::Load);

	auto pipelineInfo = stateManager.GetPipelineInfo();
	pipelineInfo->SetDepthTestEnabled(true);

	auto renderPassInfo = stateManager.GetRenderPassInfo();
	renderPassInfo->InitialRenderTargetState = ResourceState::RenderTarget;

	stateManager.SetCullMode(CullMode::Front);

	SetConstantBuffer(0, m_Renderer.GetMainConstBuffer());
	SetCombinedImageSampler(1, SkyboxPass::Main);

	SetRenderTarget(0, GBufferPass::Albedo);
	SetDepthStencil(GBufferPass::Depth);
}

void SkyboxPass::Render()
{
	//TODO: dont do like this, make one big DEVICE LOCAL buffer and calculate offsets
	m_Renderer.GetVertexUploadBuffer()->FillBuffer(skyboxVertices, 396 * sizeof(float));
	m_Renderer.BindVertexData(0);
	m_Renderer.DrawVertices(36);
}