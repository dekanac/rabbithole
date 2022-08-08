#define GLFW_INCLUDE_VULKAN
#include "Renderer.h"

#include "Core/Application.h"
#include "ECS/EntityManager.h"
#include "Input/InputManager.h"
#include "Model/Model.h"
#include "Render/Camera.h"
#include "Render/Window.h"
#include "Render/RenderPass.h"
#include "Render/ResourceStateTracking.h"
#include "Render/SuperResolutionManager.h"
#include "Render/Shader.h"
#include "Utils/utils.h"
#include "Vulkan/Include/VulkanWrapper.h"
#include "Render/PipelineManager.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>
#include <random>
#include <sstream>
#include <set>
#include <thread>
#include <mutex>
#include <filesystem>

rabbitVec3f renderDebugOption;

void Renderer::ExecuteRenderPass(RenderPass& renderpass)
{
	m_VulkanDevice.BeginLabel(GetCurrentCommandBuffer(), renderpass.GetName());

	renderpass.Setup(this);

	RSTManager.TransitionResources();
	renderpass.Render(this);
	
	if (m_RecordGPUTimeStamps)
	{
		RecordGPUTimeStamp(renderpass.GetName());
	}

	RSTManager.Reset();

	m_VulkanDevice.EndLabel(GetCurrentCommandBuffer());
}

bool Renderer::Init()
{
	MainCamera = new Camera();
	MainCamera->Init();

	m_CurrentUIState = new UIState;
	SuperResolutionManager::instance().Init(&m_VulkanDevice);

	m_ResourceManager = new ResourceManager();
	m_StateManager = new VulkanStateManager();

#ifdef RABBITHOLE_USING_IMGUI
	ImGui::CreateContext();
#endif

	InitTextures();

	renderDebugOption.x = 0.f; //we use vector4f as a default UBO element

	LoadModels();
	LoadAndCreateShaders();
	RecreateSwapchain();
	CreateCommandBuffers();

	m_GPUTimeStamps.OnCreate(&m_VulkanDevice, m_VulkanSwapchain->GetImageCount());

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		CreateGeometryDescriptors(gltfModels, i);
	}

	//GBUFFER RENDER SET
	depthStencil = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{ 
			.dimensions = {GetNativeWidth , GetNativeHeight, 1},
			.flags = {TextureFlags::DepthStencil | TextureFlags::Read},
			.format = {Format::D32_SFLOAT},
			.name = {"GBuffer DepthStencil"}
		});

	albedoGBuffer = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetNativeWidth , GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read},
			.format = {Format::B8G8R8A8_UNORM},
			.name = {"GBuffer Albedo"}
		});

	normalGBuffer = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetNativeWidth , GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"GBuffer Normal"}
		});

	worldPositionGBuffer = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetNativeWidth , GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"GBuffer World Position"}
		});

	velocityGBuffer = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetNativeWidth , GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read},
			.format = {Format::R32G32_FLOAT},
			.name = {"GBuffer Velocity"}
		});
	
	//for now max 1024 commands
	geomDataIndirectDraw = new IndexedIndirectBuffer();
	geomDataIndirectDraw->gpuBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::IndirectBuffer | BufferUsageFlags::TransferSrc},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(IndexIndirectDrawData) * 1024},
			.name = {"GeomDataIndirectDraw"}
		});

	geomDataIndirectDraw->localBuffer = (IndexIndirectDrawData*)malloc(sizeof(IndexIndirectDrawData) * 10240);

	debugTexture = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Debug Texture"}
		});
	
	debugTextureParamsBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(DebugTextureParams)},
			.name = {"Debug Texture Params"}
		});

#ifdef USE_RABBITHOLE_TOOLS
	entityHelper = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::TransferSrc},
			.format = {Format::R32_UINT},
			.name = {"Entity Helper"}
		});

	entityHelperBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::StorageBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {GetNativeWidth * GetNativeHeight * 4},
			.name = {"Entity Helper"}
		});
#endif

	shadowMap = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::Read},
			.format = {Format::R8_UNORM},
			.name = {"Shadow Map"},
			.arraySize = {MAX_NUM_OF_LIGHTS},
		});

	lightingMain = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::TransferSrc},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Lighting Main"},
		});

	//init fsr
	fsrOutputTexture = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetUpscaledWidth, GetUpscaledHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"FSR2 Output"},
		});

	//volumetricFog
	volumetricOutput = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::TransferSrc},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Volumetric Fog Output"},
		});
	
	mediaDensity3DLUT = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {160, 90, 128},
			.flags = {TextureFlags::Read | TextureFlags::TransferSrc},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Media Density"},
		});

	scatteringTexture = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {160, 90, 64},
			.flags = {TextureFlags::Read | TextureFlags::TransferSrc},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Scattering Calculation"},
		});
	
	volumetricFogParamsBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(VolumetricFogParams)},
			.name = {"Volumetric Fog Params"}
		});

	//posteffects
	postUpscalePostEffects = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetUpscaledWidth, GetUpscaledHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Post Upscale PostEffects" },
		});

	//shadow denoise

	//prepass
	const uint32_t tileW = GetCSDispatchCount(GetNativeWidth, 8);
	const uint32_t tileH = GetCSDispatchCount(GetNativeHeight, 4);

	const uint32_t tileSize = tileH * tileW;

	denoiseShadowMaskBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::StorageBuffer},
			.memoryAccess = {MemoryAccess::GPU},
			.size = {tileSize * sizeof(uint32_t)},
			.name = {"Denoise Shadow Mask Buffer"}
		});

	denoiseBufferDimensions = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(DenoiseBufferDimensions)},
			.name = {"Denoise Dimensions buffer"}
		});

	//classify

	denoiseShadowDataBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(DenoiseShadowData)},
			.name = {"Denoise Shadow Data Buffer"}
		});

	denoiseTileMetadataBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::StorageBuffer},
			.memoryAccess = {MemoryAccess::GPU},
			.size = {tileSize * sizeof(uint32_t)},
			.name = {"Denoise Metadata buffer"}
		});

	denoiseMomentsBuffer0 = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::Read},
			.format = {Format::R11G11B10_FLOAT},
			.name = {"Denoise Moments Buffer0"}
		});

	denoiseMomentsBuffer1 = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::Read},
			.format = {Format::R11G11B10_FLOAT},
			.name = {"Denoise Moments Buffer1"}
		});

	denoiseReprojectionBuffer0 = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::Read},
			.format = {Format::R16G16_FLOAT},
			.name = {"Denoise Reprojection Buffer0"}
		});

	denoiseReprojectionBuffer1 = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::Read},
			.format = {Format::R16G16_FLOAT},
			.name = {"Denoise Reprojection Buffer1"}
		});

	denoiseLastFrameDepth = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::Read},
			.format = {Format::R32_SFLOAT},
			.name = {"Denoise Last Frame Depth"}
		});

	denoiseShadowFilterDataBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(DenoiseShadowFilterData)},
			.name = {"Denoise Shadow Filter Data Buffer"}
		});

	denoisedShadowOutput = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::Read},
			.format = {Format::R16G16B16A16_UNORM},
			.name = {"Denoised Shadow Output"}
		});

	InitNoiseTextures();
	InitSSAO();
	//init acceleration structure
	InitMeshDataForCompute();
	InitLights();

	return true;
}

bool Renderer::Shutdown()
{
	//TODO: clear everything properly
	delete MainCamera;
	delete m_CurrentUIState;
	gltfModels.clear();
	m_GPUTimeStamps.OnDestroy();
	delete m_ResourceManager;
	//delete m_StateManager;
    return true;
}

void Renderer::Clear() const
{
	geomDataIndirectDraw->Reset();
}

void Renderer::Draw(float dt)
{
	m_CurrentDeltaTime = dt;

	MainCamera->Update(dt);

    DrawFrame();

	m_CurrentFrameIndex++;
	
	Clear();
}

void Renderer::DrawFrame()
{
	uint32_t imageIndex;
	auto result = m_VulkanSwapchain->AcquireNextImage(&imageIndex);
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
	{
		LOG_ERROR("failed to acquire swap chain image!");
	}

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapchain();
		return;
	}

	RecordCommandBuffer(imageIndex);

	result = m_VulkanSwapchain->SubmitCommandBuffers(&m_CommandBuffers[imageIndex], &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
	{
		m_FramebufferResized = false;
		RecreateSwapchain();
	}

	if (result != VK_SUCCESS) 
	{
		LOG_ERROR("failed to present swap chain image!");
	}
}

void Renderer::CreateGeometryDescriptors(std::vector<VulkanglTFModel>& models, uint32_t imageIndex)
{
	VulkanDescriptorSetLayout* descrSetLayout = new VulkanDescriptorSetLayout(&m_VulkanDevice, { GetShader("VS_GBuffer"), GetShader("FS_GBuffer") }, "GeometryDescSetLayout");

	VulkanDescriptorInfo descriptorinfo{};
	descriptorinfo.Type = DescriptorType::UniformBuffer;
	descriptorinfo.Binding = 0;
	descriptorinfo.buffer = m_MainConstBuffer[imageIndex];

	VulkanDescriptor* bufferDescr = new VulkanDescriptor(descriptorinfo);

	for (auto& model : models)
	{
		for (size_t i = 0; i < model.GetMaterials().size(); i++)
		{
			VulkanglTFModel::Material& modelMaterial = model.GetMaterials()[i];
			auto& modelTextures = model.GetTextures();
			auto& modelTexureIndices = model.GetTextureIndices();

			//albedo
			VulkanTexture* albedo;
			if (modelMaterial.baseColorTextureIndex != 0xFFFFFFFF && modelTextures.size() > 0)
				albedo = modelTextures[modelTexureIndices[modelMaterial.baseColorTextureIndex]];
			else
				albedo = g_DefaultWhiteTexture;

			VulkanDescriptorInfo descriptorinfo2{};
			descriptorinfo2.Type = DescriptorType::CombinedSampler;
			descriptorinfo2.Binding = 1;
			descriptorinfo2.imageView = albedo->GetView();
			descriptorinfo2.imageSampler = albedo->GetSampler();

			VulkanDescriptor* cisDescr = new VulkanDescriptor(descriptorinfo2);

			//normal
			//TODO: do something with useNormalMap bool in shaders
			VulkanTexture* normal;
			if (modelMaterial.normalTextureIndex != 0xFFFFFFFF && modelTextures.size() > 0)
				normal = modelTextures[modelTexureIndices[modelMaterial.normalTextureIndex]];
			else
				normal = g_DefaultWhiteTexture;

			VulkanDescriptorInfo descriptorinfo3{};
			descriptorinfo3.Type = DescriptorType::CombinedSampler;
			descriptorinfo3.Binding = 2;
			descriptorinfo3.imageView = normal->GetView();
			descriptorinfo3.imageSampler = normal->GetSampler();

			VulkanDescriptor* cis2Descr = new VulkanDescriptor(descriptorinfo3);

			//metallicRoughness
			VulkanTexture* metallicRoughness;
			if (modelMaterial.metallicRoughnessTextureIndex != 0xFFFFFFFF && modelTextures.size() > 0)
				metallicRoughness = modelTextures[modelTexureIndices[modelMaterial.metallicRoughnessTextureIndex]];
			else
				metallicRoughness = g_DefaultWhiteTexture;

			VulkanDescriptorInfo descriptorinfo4{};
			descriptorinfo4.Type = DescriptorType::CombinedSampler;
			descriptorinfo4.Binding = 3;
			descriptorinfo4.imageView = metallicRoughness->GetView();
			descriptorinfo4.imageSampler = metallicRoughness->GetSampler();

			VulkanDescriptor* cis3Descr = new VulkanDescriptor(descriptorinfo4);

			VulkanDescriptorSet* descriptorSet = new VulkanDescriptorSet(&m_VulkanDevice, m_DescriptorPool.get(), descrSetLayout, { bufferDescr, cisDescr, cis2Descr, cis3Descr }, "GeometryDescSet");

			modelMaterial.materialDescriptorSet[imageIndex] = descriptorSet;
		}
	}

}

void Renderer::InitImgui()
{
	VulkanDescriptorPoolSize cisSize;
	cisSize.Count = 100;
	cisSize.Type = DescriptorType::CombinedSampler;
	VulkanDescriptorPoolSize bufferSize;
	bufferSize.Count = 100;
	bufferSize.Type = DescriptorType::UniformBuffer;

	VulkanDescriptorPoolInfo info{};
	info.DescriptorSizes = { cisSize, bufferSize };
	info.MaxSets = 100;

	VulkanDescriptorPool* imguiDescriptorPool = new VulkanDescriptorPool(&m_VulkanDevice, info);

	//this initializes imgui for GLFW
	ImGui_ImplGlfw_InitForVulkan(Window::instance().GetNativeWindowHandle(), true);

	ImGui_ImplVulkan_InitInfo init_info = {};
	m_VulkanDevice.InitImguiForVulkan(init_info);
	init_info.DescriptorPool = imguiDescriptorPool->GetPool();
	init_info.MinImageCount = m_VulkanSwapchain->GetImageCount();
	init_info.ImageCount = m_VulkanSwapchain->GetImageCount();

	ImGui_ImplVulkan_Init(&init_info, m_StateManager->GetRenderPass()->GetVkRenderPass());

	auto commandBuffer = m_VulkanDevice.BeginSingleTimeCommands();
	ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
	m_VulkanDevice.EndSingleTimeCommands(commandBuffer);

	//clear font textures from cpu data
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	ImGui::StyleColorsDark();
}

void Renderer::InitTextures()
{
	g_DefaultWhiteTexture = m_ResourceManager->CreateTexture(m_VulkanDevice, ROTextureCreateInfo{
			.filePath = {"res/textures/default_white.png"},
			.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst},
			.format = {Format::R8G8B8A8_UNORM_SRGB},
			.name = {"Default White"}
		});
	
	g_DefaultBlackTexture = m_ResourceManager->CreateTexture(m_VulkanDevice, ROTextureCreateInfo{
			.filePath = {"res/textures/default_black.png"},
			.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst},
			.format = {Format::R8G8B8A8_UNORM_SRGB},
			.name = {"Default Black"}
		});
	
	g_Default3DTexture = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {4, 4, 4},
			.flags = {TextureFlags::Color | TextureFlags::Read},
			.format = {Format::B8G8R8A8_UNORM},
			.name = {"Default 3D"}
		});
	
	g_DefaultArrayTexture = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {4, 4, 1},
			.flags = {TextureFlags::Color | TextureFlags::Read},
			.format = {Format::B8G8R8A8_UNORM},
			.name = {"Default Array"},
			.arraySize = {4}
		});

	skyboxTexture = m_ResourceManager->CreateTexture(m_VulkanDevice, ROTextureCreateInfo{
			.filePath = {"res/textures/skybox/skybox.jpg"},
			.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::CubeMap | TextureFlags::TransferDst},
			.format = {Format::R8G8B8A8_UNORM_SRGB},
			.name = {"Skybox"}
		});
}

void Renderer::InitNoiseTextures()
{
	noise2DTexture = m_ResourceManager->CreateTexture(m_VulkanDevice, ROTextureCreateInfo{
			.filePath = {"res/textures/noise3.png"},
			.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst},
			.format = {Format::B8G8R8A8_UNORM},
			.name = {"Noise2D"}
		});
	
	blueNoise2DTexture = m_ResourceManager->CreateTexture(m_VulkanDevice, ROTextureCreateInfo{
		.filePath = {"res/textures/noise.png"},
		.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst},
		.format = {Format::B8G8R8A8_UNORM},
		.name = {"BlueNoise2D"}
		});

	noise3DLUT = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {256, 256, 256},
			.flags = {TextureFlags::Color | TextureFlags::Read},
			.format = {Format::R32_SFLOAT},
			.name = {"Noise 3D LUT"}
		});
}

void Renderer::LoadModels()
{
	gltfModels.emplace_back(&m_VulkanDevice, "res/meshes/separateObjects.gltf");
	//gltfModels.emplace_back(&m_VulkanDevice, "res/meshes/cottage.gltf");
	//gltfModels.emplace_back(&m_VulkanDevice, "res/meshes/sponza/sponza.gltf");
}

void Renderer::BeginRenderPass(VkExtent2D extent)
{
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_StateManager->GetRenderPass()->GetVkRenderPass();
	renderPassInfo.framebuffer = m_StateManager->GetFramebuffer()->GetVkFramebuffer();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = extent;

	auto renderTargetCount = m_StateManager->GetRenderTargetCount();
	auto& renderTargets = m_StateManager->GetRenderTargets();

	std::vector<VkClearValue> clearValues(renderTargetCount);

	for (size_t i = 0; i < renderTargetCount; i++)
	{
		auto format = renderTargets[i]->GetFormat();
		clearValues[i] = GetVkClearColorValueFor(format);
	}

	if (m_StateManager->HasDepthStencil())
	{
		renderTargetCount++;
		VkClearValue depthClearValue{};
		depthClearValue.depthStencil = { 1.0f, 0 }; //for now this is it, add function to support other DS formats like in GetVkClearColorValueForFormat
		clearValues.push_back(depthClearValue);
	}

	renderPassInfo.clearValueCount = static_cast<uint32_t>(renderTargetCount);
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(GetCurrentCommandBuffer(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void Renderer::EndRenderPass()
{
	vkCmdEndRenderPass(GetCurrentCommandBuffer());
	m_StateManager->Reset();
}

void Renderer::RecordGPUTimeStamp(const char* label)
{
	m_GPUTimeStamps.GetTimeStamp(GetCurrentCommandBuffer(), label);
}

void Renderer::BindGraphicsPipeline(bool isPostUpscale)
{
	//if pipeline is not dirty bind the one already in state manager
	if (!m_StateManager->GetPipelineDirty())
	{
		m_StateManager->GetPipeline()->Bind(GetCurrentCommandBuffer());
		return;
	}

	auto pipelineInfo = m_StateManager->GetPipelineInfo();
	auto& attachments = m_StateManager->GetRenderTargets();
	auto depthStencil = m_StateManager->GetDepthStencil();
	auto renderPassInfo = m_StateManager->GetRenderPassInfo();

	//renderpass
	VulkanRenderPass* renderpass = PipelineManager::instance().FindOrCreateRenderPass(m_VulkanDevice, attachments, depthStencil, *renderPassInfo);
	m_StateManager->SetRenderPass(renderpass);

	//framebuffer
	VulkanFramebufferInfo framebufferInfo{};
	framebufferInfo.height = isPostUpscale ? GetUpscaledHeight : GetNativeHeight;
	framebufferInfo.width = isPostUpscale ? GetUpscaledWidth : GetNativeWidth;
	VulkanFramebuffer* framebuffer = PipelineManager::instance().FindOrCreateFramebuffer(m_VulkanDevice, attachments, depthStencil, renderpass, framebufferInfo);
	m_StateManager->SetFramebuffer(framebuffer);

	//pipeline
	pipelineInfo->renderPass = m_StateManager->GetRenderPass();
	auto pipeline = PipelineManager::instance().FindOrCreateGraphicsPipeline(m_VulkanDevice, *pipelineInfo);
	m_StateManager->SetPipeline(pipeline);

	pipeline->Bind(GetCurrentCommandBuffer());
	m_StateManager->SetPipelineDirty(false);

}

void Renderer::BindComputePipeline()
{
	if (m_StateManager->GetPipelineDirty())
	{ 
		auto pipelineInfo = m_StateManager->GetPipelineInfo();
		VulkanPipeline* computePipeline = PipelineManager::instance().FindOrCreateComputePipeline(m_VulkanDevice, *pipelineInfo);
		m_StateManager->SetPipeline(computePipeline);

		//TODO: need a better way, this is kinda reset for pipeline info because compute bind is happening in the middle of render pass independently
		computePipeline->Bind(GetCurrentCommandBuffer());

		m_StateManager->SetComputeShader(nullptr);
		m_StateManager->SetPipelineDirty(false);
	}
}

void Renderer::DrawGeometryGLTF(std::vector<VulkanglTFModel>& bucket)
{
	BindGraphicsPipeline();

	BeginRenderPass({ GetNativeWidth, GetNativeHeight });

	BindUBO();

	int currentModel = 0;

	const VkPipelineLayout* pipelineLayout = m_StateManager->GetPipeline()->GetPipelineLayout();

	for (auto& model : bucket)
	{
		model.BindBuffers(GetCurrentCommandBuffer());

		model.Draw(GetCurrentCommandBuffer(), pipelineLayout, m_CurrentImageIndex, geomDataIndirectDraw);
	}

    geomDataIndirectDraw->SubmitToGPU();

	EndRenderPass();
}

/*
void Renderer::DrawBoundingBoxes(std::vector<RabbitModel*>& bucket)
{
	BindGraphicsPipeline();

	BindDescriptorSets();

	BeginRenderPass({ GetNativeWidth, GetNativeHeight });

	BindUBO();

	for (auto model : bucket)
	{
		//TODO: implement this
		std::vector<Vertex> vertex;

		rabbitVec3f a = model->m_BoundingBox.bounds[0];
		rabbitVec3f f = model->m_BoundingBox.bounds[1];

		//TODO: what are you doin' boi
		{
			vertex.push_back(Vertex{ a });
			vertex.push_back(Vertex{ {f.x, a.y, a.z} }); //b

			vertex.push_back(Vertex{ {f.x, a.y, a.z} }); //b
			vertex.push_back(Vertex{ {f.x, a.y, f.z} }); //c

			vertex.push_back(Vertex{ {f.x, a.y, f.z} }); //c
			vertex.push_back(Vertex{ {a.x, a.y, f.z} }); //d

			vertex.push_back(Vertex{ {a.x, a.y, f.z} }); //d
			vertex.push_back(Vertex{ a });

			vertex.push_back(Vertex{ f });
			vertex.push_back(Vertex{ {a.x, f.y, f.z} }); //e

			vertex.push_back(Vertex{ {a.x, f.y, f.z} }); //e
			vertex.push_back(Vertex{ {a.x, f.y, a.z} }); //h

			vertex.push_back(Vertex{ {a.x, f.y, a.z} }); //h
			vertex.push_back(Vertex{ {f.x, f.y, a.z} }); //g

			vertex.push_back(Vertex{ {f.x, f.y, a.z} }); //g
			vertex.push_back(Vertex{ f });

			vertex.push_back(Vertex{ a });
			vertex.push_back(Vertex{ {a.x, f.y, a.z} }); //h

			vertex.push_back(Vertex{ {f.x, a.y, a.z} }); //b
			vertex.push_back(Vertex{ {f.x, f.y, a.z} }); //g

			vertex.push_back(Vertex{ {f.x, a.y, f.z} }); //c
			vertex.push_back(Vertex{ f });

			vertex.push_back(Vertex{ {a.x, a.y, f.z} }); //d
			vertex.push_back(Vertex{ {a.x, f.y, f.z} }); //e
		}
		
		m_VertexUploadBuffer->FillBuffer(vertex.data(), this offset is from skybox, TODO: automate this sizeof(Vertex) * 36, vertex.size() * sizeof(Vertex));

		BindVertexData(sizeof(Vertex) * 36);

		BindModelMatrix(model);

		vkCmdDraw(GetCurrentCommandBuffer(), 24, 1, 0, 0);
	}

	EndRenderPass();
}
*/

void Renderer::BeginCommandBuffer()
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VULKAN_API_CALL(vkBeginCommandBuffer(GetCurrentCommandBuffer(), &beginInfo), "failed to begin recording command buffer!");
}

void Renderer::EndCommandBuffer()
{
	VULKAN_API_CALL(vkEndCommandBuffer(GetCurrentCommandBuffer()), "failed to end recording command buffer!");
}

void Renderer::FillTheLightParam(LightParams& lightParam, rabbitVec3f position, rabbitVec3f color, float radius, float intensity, LightType type)
{
	lightParam.position[0] = position.x;
	lightParam.position[1] = position.y;
	lightParam.position[2] = position.z;
	lightParam.color[0] = color.x;
	lightParam.color[1] = color.y;
	lightParam.color[2] = color.z;
	lightParam.radius = radius;
	lightParam.intensity = intensity;
	lightParam.type = (uint32_t)type;
}

std::vector<char> Renderer::ReadFile(const std::string& filepath)
{
	std::ifstream file{ filepath, std::ios::ate | std::ios::binary };

	if (!file.is_open())
	{
		LOG_ERROR("failed to open file: " + filepath);
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();
	return buffer;
}

void Renderer::DrawFullScreenQuad(bool isPostUpscale)
{
	BindGraphicsPipeline(isPostUpscale);

	BindDescriptorSets();

	Extent2D renderExtent{};
	renderExtent.width = isPostUpscale ? GetUpscaledWidth : GetNativeWidth;
	renderExtent.height = isPostUpscale ? GetUpscaledHeight : GetNativeHeight;

	BeginRenderPass(renderExtent);

	vkCmdDraw(GetCurrentCommandBuffer(), 3, 1, 0, 0);

	EndRenderPass();
}

void Renderer::UpdateEntityPickId()
{
	//steal input comp from camera to retrieve mouse pos
	auto cameras = EntityManager::instance().GetAllEntitiesWithComponent<CameraComponent>();
	auto inputComponent = cameras[0]->GetComponent<InputComponent>();
	auto x = inputComponent->mouse_current_x;
	auto y = inputComponent->mouse_current_y;

	if (!(x >= 0 && y >= 0 && x <= GetNativeWidth && y <= GetNativeHeight))
	{
		x = 0; //TODO: fix this. not a great solution but ok
		y = 0;
	}

	PushMousePos push{};
	push.x = x;
	push.y = y;
	BindPushConstant(push);

}

void Renderer::CopyToSwapChain()
{
	BindGraphicsPipeline(true);

#ifdef RABBITHOLE_USING_IMGUI
	if (!m_ImguiInitialized)
	{
		InitImgui();
		RegisterTexturesToImGui();
		m_ImguiInitialized = true;
	}
#endif

	BindDescriptorSets();

	BeginRenderPass({ GetUpscaledWidth, GetUpscaledHeight });

	//COPY TO SWAPCHAIN
	vkCmdDraw(GetCurrentCommandBuffer(), 3, 1, 0, 0);

#ifdef RABBITHOLE_USING_IMGUI
	//DRAW UI(imgui)
	if (m_ImguiInitialized && imguiReady)
	{
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), GetCurrentCommandBuffer());
	}
#endif

	EndRenderPass();
}

rabbitMat4f prevViewProjMatrix;
bool hasViewProjMatrixChanged = false;
void Renderer::BindCameraMatrices(Camera* camera)
{	
	m_StateManager->UpdateUBOElement(UBOElement::PrevViewProjMatrix, 4, &prevViewProjMatrix);

	rabbitMat4f viewMatrix = camera->View();
	m_StateManager->UpdateUBOElement(UBOElement::ViewMatrix, 4, &viewMatrix);

	rabbitMat4f projMatrix = camera->Projection();
	m_StateManager->UpdateUBOElement(UBOElement::ProjectionMatrix, 4, &projMatrix);

	//todo: double check this, for now I use jittered matrix in VS_Gbuffer FS_SSAO and VS_Skybox
	rabbitMat4f projMatrixJittered = camera->ProjectionJittered();
	m_StateManager->UpdateUBOElement(UBOElement::ProjectionMatrixJittered, 4, &projMatrixJittered);

	rabbitMat4f viewProjMatrix = projMatrix * viewMatrix;
	m_StateManager->UpdateUBOElement(UBOElement::ViewProjMatrix, 4, &viewProjMatrix);

	rabbitVec3f cameraPos = camera->GetPosition();
	m_StateManager->UpdateUBOElement(UBOElement::CameraPosition, 1, &cameraPos);
	
	rabbitMat4f viewInverse = glm::inverse(viewMatrix);
	m_StateManager->UpdateUBOElement(UBOElement::ViewInverse, 4, &viewInverse);

	rabbitMat4f projInverse = glm::inverse(projMatrix);
	m_StateManager->UpdateUBOElement(UBOElement::ProjInverse, 4, &projInverse);

	rabbitMat4f viewProjInverse = viewInverse * projInverse;
	m_StateManager->UpdateUBOElement(UBOElement::ViewProjInverse, 4, &viewProjInverse);

	float width = projMatrix[0][0];
	float height = projMatrix[1][1];

	rabbitVec4f eyeXAxis = viewInverse * rabbitVec4f(-1.0 / width, 0, 0, 0);
	rabbitVec4f eyeYAxis = viewInverse * rabbitVec4f(0, -1.0 / height, 0, 0);
	rabbitVec4f eyeZAxis = viewInverse * rabbitVec4f(0, 0, 1.f, 0);

	m_StateManager->UpdateUBOElement(UBOElement::EyeXAxis, 1, &eyeXAxis);
	m_StateManager->UpdateUBOElement(UBOElement::EyeYAxis, 1, &eyeYAxis);
	m_StateManager->UpdateUBOElement(UBOElement::EyeZAxis, 1, &eyeZAxis);

	if (prevViewProjMatrix == viewProjMatrix)
		hasViewProjMatrixChanged = false;
	else
		hasViewProjMatrixChanged = true;

	prevViewProjMatrix = viewProjMatrix;
}

void Renderer::BindUBO()
{	
	if (m_StateManager->GetUBODirty())
	{ 
		m_MainConstBuffer[m_CurrentImageIndex]->FillBuffer(m_StateManager->GetUBO(), sizeof(UniformBufferObject));
		
		m_StateManager->SetUBODirty(false);
	}
}

void Renderer::BindDescriptorSets()
{
	auto descriptorSet = m_StateManager->FinalizeDescriptorSet(m_VulkanDevice, m_DescriptorPool.get());
	//TODO: decide where to store descriptor sets, models or state manager?
	auto pipeline = m_StateManager->GetPipeline();
	auto bindPoint = pipeline->GetType() == PipelineType::Graphics ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE;

	vkCmdBindDescriptorSets(GetCurrentCommandBuffer(), bindPoint, *pipeline->GetPipelineLayout(), 0, 1, descriptorSet->GetVkDescriptorSet(), 0, nullptr);
}

void Renderer::LoadAndCreateShaders()
{
	//TODO: implement real shader compiler and stuff
	std::filesystem::path currentPath = std::filesystem::current_path();
	currentPath += "\\res\\shaders";

	for (const auto& file : std::filesystem::directory_iterator(currentPath))
	{
		std::string filePath = file.path().string();
		auto foundLastSlash = filePath.find_last_of("/\\");

		if (foundLastSlash)
		{
			std::string fileNameWithExt = filePath.substr(foundLastSlash + 1);
			auto foundLastDot = fileNameWithExt.find_last_of(".");

			if (foundLastDot)
			{
				std::string fileExtension = fileNameWithExt.substr(foundLastDot + 1);

				//for spv files create shader modules
				if (fileExtension.compare("spv") == 0)
				{
					std::string fileNameFinal = fileNameWithExt.substr(0, foundLastDot);

					auto shaderCode = ReadFile(filePath);
					
					ShaderInfo createInfo{};
					createInfo.CodeEntry = nullptr;

					switch (fileNameFinal[0])
					{
					case 'C':
						createInfo.Type = ShaderType::Compute;
						break;
					case 'V':
						createInfo.Type = ShaderType::Vertex;
						break;
					case 'F':
						createInfo.Type = ShaderType::Fragment;
						break;
					default:
						LOG_ERROR("Unrecognized shader stage! Should be CS, VS or FS");
						break;
					}

					m_ResourceManager->CreateShader(m_VulkanDevice, createInfo, shaderCode, fileNameFinal.c_str());
				}
			}
		}
	}
}

void Renderer::CreateCommandBuffers() 
{
	m_CommandBuffers.resize(m_VulkanSwapchain->GetImageCount());

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_VulkanDevice.GetCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

	if (vkAllocateCommandBuffers(m_VulkanDevice.GetGraphicDevice(), &allocInfo, m_CommandBuffers.data()) !=
		VK_SUCCESS) 
	{
		LOG_ERROR("failed to allocate command buffers!");
	}
}

void Renderer::RecreateSwapchain()
{
	Extent2D extent{};
	extent.width = GetUpscaledWidth;
	extent.height = GetUpscaledHeight;

	vkDeviceWaitIdle(m_VulkanDevice.GetGraphicDevice());
	m_VulkanSwapchain = std::make_unique<VulkanSwapchain>(m_VulkanDevice, extent);

	CreateUniformBuffers();
	CreateDescriptorPool();
}

void Renderer::ResourceBarrier(VulkanTexture* texture, ResourceState oldLayout, ResourceState newLayout, ResourceStage srcStage, ResourceStage dstStage)
{
	VkCommandBuffer commandBuffer = GetCurrentCommandBuffer();

	uint32_t arraySize = texture->GetResource()->GetInfo().ArraySize;

	bool isDepth = 
		(oldLayout == ResourceState::DepthStencilRead || oldLayout == ResourceState::DepthStencilWrite) ||
		(newLayout == ResourceState::DepthStencilRead || newLayout == ResourceState::DepthStencilWrite);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = GetVkImageLayoutFrom(oldLayout);
	barrier.newLayout = GetVkImageLayoutFrom(newLayout);
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = texture->GetResource()->GetImage();
	barrier.subresourceRange.aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = arraySize;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	//if (oldLayout == ResourceState::None && newLayout == ResourceState::TransferDst) {
	//	barrier.srcAccessMask = 0;
	//	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	//
	//	sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	//	destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	//}
	//else if (oldLayout == ResourceState::RenderTarget && newLayout == ResourceState::TransferSrc) {
	//	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	//	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	//
	//	sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	//	destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	//}
	//else if (oldLayout == ResourceState::TransferSrc && newLayout == ResourceState::RenderTarget) {
	//	barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	//	barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	//
	//	sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	//	destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	//}
	//
	//else if (oldLayout == ResourceState::GenericRead && newLayout == ResourceState::TransferSrc) {
	//	barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	//	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	//
	//	sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	//	destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	//}
	//else if (oldLayout == ResourceState::TransferSrc && newLayout == ResourceState::GenericRead) {
	//	barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	//	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	//
	//	sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	//	destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	//}
	//else if (oldLayout == ResourceState::GenericRead && newLayout == ResourceState::TransferDst) {
	//	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	//	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	//
	//	sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	//	destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	//}
	//else if (oldLayout == ResourceState::TransferDst && newLayout == ResourceState::GenericRead)
	//{
	//	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	//	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	//
	//	sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	//	destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	//}
	//else if (oldLayout == ResourceState::TransferDst && newLayout == ResourceState::GeneralCompute)
	//{
	//	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	//	barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
	//
	//	sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	//	destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	//}
	//else if (oldLayout == ResourceState::None && newLayout == ResourceState::RenderTarget)
	//{
	//	barrier.srcAccessMask = 0;
	//	barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	//
	//	sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	//	destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	//}
	//else if (oldLayout == ResourceState::RenderTarget && newLayout == ResourceState::GenericRead)
	//{
	//	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	//	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	//
	//	sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	//	destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	//}
	//else if (oldLayout == ResourceState::GenericRead && newLayout == ResourceState::RenderTarget)
	//{
	//	barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	//	barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	//
	//	sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	//	destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	//}
	if (oldLayout == ResourceState::None && newLayout == ResourceState::DepthStencilWrite)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == ResourceState::DepthStencilWrite && newLayout == ResourceState::GenericRead)
	{
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == ResourceState::DepthStencilRead && newLayout == ResourceState::GenericRead)
	{
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == ResourceState::DepthStencilRead && newLayout == ResourceState::DepthStencilWrite)
	{
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == ResourceState::GenericRead && newLayout == ResourceState::DepthStencilWrite)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	//else if (oldLayout == ResourceState::GenericRead && newLayout == ResourceState::GeneralCompute)
	//{
	//	barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	//	barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
	//
	//	sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	//	destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	//}
	//else if (oldLayout == ResourceState::RenderTarget && newLayout == ResourceState::GeneralCompute)
	//{
	//	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	//	barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
	//
	//	sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	//	destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	//}
	//else if (oldLayout == ResourceState::GeneralCompute && newLayout == ResourceState::RenderTarget)
	//{
	//	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
	//	barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	//	
	//	sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	//	destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	//}
	//else if (oldLayout == ResourceState::GeneralCompute && newLayout == ResourceState::GenericRead)
	//{
	//	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
	//	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	//
	//	sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	//	destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	//}
	//else
	//{
	//	LOG_ERROR("unsupported layout transition!");
	//}

	if (!isDepth)
	{
		barrier.srcAccessMask = GetVkAccessFlagsFromResourceState(oldLayout);
		barrier.dstAccessMask = GetVkAccessFlagsFromResourceState(newLayout);

		sourceStage =			GetVkPipelineStageFromResourceStage(srcStage);
		destinationStage =		GetVkPipelineStageFromResourceStage(dstStage);
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	texture->SetResourceState(newLayout);
}

void Renderer::UpdateDebugOptions()
{
	if (InputManager::instance().IsButtonActionActive("Debug1", EInputActionState::Pressed))
	{
		renderDebugOption.x = 1.f;
	}
	else if (InputManager::instance().IsButtonActionActive("Debug2", EInputActionState::Pressed))
	{
		renderDebugOption.x = 2.f;
	}
	else if (InputManager::instance().IsButtonActionActive("Debug3", EInputActionState::Pressed))
	{
		renderDebugOption.x = 3.f;
	}
	else if (InputManager::instance().IsButtonActionActive("Debug4", EInputActionState::Pressed))
	{
		renderDebugOption.x = 0.f;
	}

	m_StateManager->UpdateUBOElement(UBOElement::DebugOption, 1, &renderDebugOption);

}
void Renderer::RecordCommandBuffer(int imageIndex)
{
	SetCurrentImageIndex(imageIndex);
	BeginCommandBuffer();

	std::vector<TimeStamp> timeStamps{};

	if (m_RecordGPUTimeStamps)
	{
		m_GPUTimeStamps.OnBeginFrame(GetCurrentCommandBuffer(), &timeStamps);
		m_GPUTimeStamps.GetTimeStamp(GetCurrentCommandBuffer(), "Begin of the frame");
	}

	if (m_ImguiInitialized)
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Main debug frame");
		ImGui::Checkbox("GPU Profiler Enabled: ", &m_RecordGPUTimeStamps);
#ifdef USE_RABBITHOLE_TOOLS
		ImGui::Checkbox("Draw Bounding Box", &m_DrawBoundingBox);
		ImGui::Checkbox("Enable Entity picking", &m_RenderOutlinedEntity);
#endif

		ImGui::End();

		ImGuiTextureDebugger();

		if (m_RecordGPUTimeStamps)
		{
			ImguiProfilerWindow(timeStamps);
		}

		imguiReady = true;
	}

	Create3DNoiseTexturePass noise3DLUTGeneration{};
	GBufferPass gbuffer{};
	BoundingBoxPass bbPass{};
	SSAOPass ssao{};
	SSAOBlurPass ssaoBlur{};
	RTShadowsPass RTShadows{};
	ShadowDenoisePrePass shadowDenoisePrepass{};
	ShadowDenoiseTileClassificationPass shadowDenoiseTileClassification{};
	ShadowDenoiseFilterPass0 shadowDenoiseFilter0{};
	ShadowDenoiseFilterPass1 shadowDenoiseFilter1{};
	ShadowDenoiseFilterPass2 shadowDenoiseFilter2{};
	VolumetricPass volumetric{};
	ComputeScatteringPass computeScattering{};
	LightingPass lighting{};
	ApplyVolumetricFogPass applyVolumerticFog{};
	SkyboxPass skybox{};
	OutlineEntityPass outlineEntity{};
	FSR2Pass fsr2{};
	TonemappingPass tonemapping{};
	TextureDebugPass textureDebug{};
	CopyToSwapchainPass copyToSwapchain{};

	UpdateUIStateAndFSR2PreDraw();
	UpdateDebugOptions();
	UpdateConstantBuffer();

	//replace with call once impl
	if (!init3dnoise)
	{
		ExecuteRenderPass(noise3DLUTGeneration);
		init3dnoise = true;
	}

	ExecuteRenderPass(gbuffer);
	ExecuteRenderPass(ssao);
	ExecuteRenderPass(ssaoBlur);
	ExecuteRenderPass(RTShadows);
	ExecuteRenderPass(shadowDenoisePrepass);
	ExecuteRenderPass(shadowDenoiseTileClassification);
	ExecuteRenderPass(shadowDenoiseFilter0);
	ExecuteRenderPass(shadowDenoiseFilter1);
	ExecuteRenderPass(shadowDenoiseFilter2);
	ExecuteRenderPass(volumetric);
	ExecuteRenderPass(computeScattering);
	ExecuteRenderPass(skybox);
	ExecuteRenderPass(lighting);
	ExecuteRenderPass(applyVolumerticFog);
    ExecuteRenderPass(textureDebug);

#ifdef USE_RABBITHOLE_TOOLS
	if (m_RenderOutlinedEntity)
	{
		ExecuteRenderPass(outlineEntity);
	}

	if (m_DrawBoundingBox)
	{ 
		ExecuteRenderPass(bbPass);
	}
#endif

	ExecuteRenderPass(fsr2);

	ExecuteRenderPass(tonemapping);

	ExecuteRenderPass(copyToSwapchain);

	if (m_RecordGPUTimeStamps)
	{
		m_GPUTimeStamps.OnEndFrame();
	}

	EndCommandBuffer();
}


void Renderer::CreateUniformBuffers()
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_MainConstBuffer[i] = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
				.flags = {BufferUsageFlags::UniformBuffer},
				.memoryAccess = {MemoryAccess::CPU2GPU},
				.size = {sizeof(UniformBufferObject)},
				.name = {"MainConstantBuffer"}
			});
	}

	m_LightParams = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(LightParams) * MAX_NUM_OF_LIGHTS},
			.name = {"Light params"}
		});

	m_VertexUploadBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::VertexBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {MB_16},
			.name = {"Vertex Upload"}
		});
}

void Renderer::CreateDescriptorPool()
{
	//TODO: see what to do with this, now its hard coded number of descriptors
	VulkanDescriptorPoolSize uboPoolSize{};
	uboPoolSize.Count = 300;
	uboPoolSize.Type = DescriptorType::UniformBuffer;

	VulkanDescriptorPoolSize samImgPoolSize{};
	samImgPoolSize.Count = 300;
	samImgPoolSize.Type = DescriptorType::SampledImage;

	VulkanDescriptorPoolSize sPoolSize{};
	sPoolSize.Count = 300;
	sPoolSize.Type = DescriptorType::Sampler;

	VulkanDescriptorPoolSize cisPoolSize{};
	cisPoolSize.Count = 300;
	cisPoolSize.Type = DescriptorType::CombinedSampler;

	VulkanDescriptorPoolSize siPoolSize{};
	siPoolSize.Count = 300;
	siPoolSize.Type = DescriptorType::StorageImage;

	VulkanDescriptorPoolSize sbPoolSize{};
	sbPoolSize.Count = 300;
	sbPoolSize.Type = DescriptorType::StorageBuffer;

	VulkanDescriptorPoolInfo vulkanDescriptorPoolInfo{};

	vulkanDescriptorPoolInfo.DescriptorSizes = { uboPoolSize, cisPoolSize, siPoolSize, sbPoolSize, samImgPoolSize, sPoolSize };

	vulkanDescriptorPoolInfo.MaxSets = 1000;

	m_DescriptorPool = std::make_unique<VulkanDescriptorPool>(&m_VulkanDevice, vulkanDescriptorPoolInfo);
}

//SSAO
float lerp(float a, float b, float f)
{
	return a + f * (b - a);
}
void Renderer::InitSSAO()
{
	SSAOParamsBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(SSAOParams)},
			.name = {"SSAO Params"}
		});

	SSAOTexture = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read},
			.format = {Format::R32_SFLOAT},
			.name = {"SSAO Main"}
		});

	SSAOBluredTexture = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetNativeWidth, GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read},
			.format = {Format::R32_SFLOAT},
			.name = {"SSAO Blured"}
		});

	std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
	std::default_random_engine generator;
	std::vector<glm::vec4> ssaoKernel;
	for (unsigned int i = 0; i < 64; ++i)
	{
		glm::vec4 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator), 0.0f);
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / 64.0;

		// scale samples s.t. they're more aligned to center of kernel
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssaoKernel.push_back(sample);
	}

	uint32_t bufferSize = 1024;
	SSAOSamplesBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {bufferSize},
			.name = {"SSAO Samples"}
		});

	SSAOSamplesBuffer->FillBuffer(ssaoKernel.data(), bufferSize);

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

	SSAONoiseTexture = new VulkanTexture(&m_VulkanDevice, &texData, TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst, Format::R32G32B32A32_FLOAT, "SSAO Noise");

	//init ssao params
	ssaoParams.radius = 0.5f;
	ssaoParams.bias = 0.025f;
	ssaoParams.resWidth = GetNativeWidth;
	ssaoParams.resHeight = GetNativeHeight;
	ssaoParams.power = 1.75f;
	ssaoParams.kernelSize = 48;
	ssaoParams.ssaoOn = true;
}

void Renderer::InitLights()
{
	FillTheLightParam(lightParams[0], { 70.0f, 300.75f, -20.75f }, { 1.f, 0.6f, 0.2f }, 25.f, 1.f, LightType_Directional);
	FillTheLightParam(lightParams[1], { 5.0f, 20.0f, -1.0f }, { 0.95f, 0.732f, 0.36f }, 0.f, 1.f, LightType_Point);
	FillTheLightParam(lightParams[2], { -10.0f, 10.0f, -10.0f }, { 1.f, 0.6f, 0.2f }, 0.f, 1.f, LightType_Point);
	FillTheLightParam(lightParams[3], { 5.f, 5.0f, 4.f }, { 1.f, 1.0f, 1.0f }, 0.f, 1.f, LightType_Point);
}

void Renderer::InitMeshDataForCompute()
{
	std::vector<Triangle> triangles;
	std::vector<rabbitVec4f> verticesFinal;
	std::vector<bool> verticesMultipliedWithMatrix;

	uint32_t offset = 0;

	auto& model = gltfModels[0];
	auto modelVertexBuffer = model.GetVertexBuffer();
	auto modelIndexBuffer = model.GetIndexBuffer();

	VulkanBuffer stagingBuffer(m_VulkanDevice, BufferUsageFlags::TransferDst, MemoryAccess::CPU, modelVertexBuffer->GetSize(), "StagingBuffer");
	m_VulkanDevice.CopyBuffer(modelVertexBuffer->GetBuffer(), stagingBuffer.GetBuffer(), modelVertexBuffer->GetSize());

	Vertex* vertexBufferCpu = (Vertex*)stagingBuffer.Map();
	uint32_t vertexCount = modelVertexBuffer->GetSize() / sizeof(Vertex);
	verticesMultipliedWithMatrix.resize(vertexCount);

	VulkanBuffer stagingBuffer2(m_VulkanDevice, BufferUsageFlags::TransferDst, MemoryAccess::CPU, modelIndexBuffer->GetSize(), "StagingBuffer");
	m_VulkanDevice.CopyBuffer(modelIndexBuffer->GetBuffer(), stagingBuffer2.GetBuffer(), modelIndexBuffer->GetSize());

	uint32_t* indexBufferCpu = (uint32_t*)stagingBuffer2.Map();
	uint32_t indexCount = modelIndexBuffer->GetSize() / sizeof(uint32_t);
	
	for (auto node : model.GetNodes())
	{ 
		glm::mat4 nodeMatrix = node.matrix;
		VulkanglTFModel::Node* currentParent = node.parent;
	
		while (currentParent)
		{
			nodeMatrix = currentParent->matrix * nodeMatrix;
		    currentParent = currentParent->parent;
		}

		for (auto& primitive : node.mesh.primitives)
		{
			for (int i = primitive.firstIndex; i < primitive.firstIndex + primitive.indexCount; i++)
			{
				auto currentIndex = indexBufferCpu[i];
				if (!verticesMultipliedWithMatrix[currentIndex])
				{
					vertexBufferCpu[currentIndex].position = nodeMatrix * rabbitVec4f { vertexBufferCpu[currentIndex].position, 1 };
					verticesMultipliedWithMatrix[currentIndex] = true;
				}
			}
		}
	}

	//get rid of this 
	for (int k = 0; k < vertexCount; k++)
	{
		rabbitVec4f position = rabbitVec4f{ vertexBufferCpu[k].position, 1.f };
		verticesFinal.push_back(position);
	}

	////will not work for multiple objects, need to calculate offset of vertex buffer all vertices + current index
	for (int j = 0; j < indexCount; j += 3)
	{
		Triangle tri;
		tri.indices[0] = offset + indexBufferCpu[j];
		tri.indices[1] = offset + indexBufferCpu[j + 1];
		tri.indices[2] = offset + indexBufferCpu[j + 2];

		triangles.push_back(tri);
	}

	std::cout << triangles.size() << " triangles!" << std::endl;

	auto node = CreateBVH(verticesFinal, triangles);

	uint32_t* triIndices;
	uint32_t indicesNum = 0;
		
	CacheFriendlyBVHNode* root;
	uint32_t nodeNum = 0;

	CreateCFBVH(triangles.data(), node, &triIndices, &indicesNum, &root, &nodeNum);
	
	vertexBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::StorageBuffer},
			.memoryAccess = {MemoryAccess::GPU},
			.size = {(uint32_t)verticesFinal.size() * sizeof(rabbitVec4f)},
			.name = {"VertexBuffer"}
		});

	trianglesBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::StorageBuffer},
			.memoryAccess = {MemoryAccess::GPU},
			.size = {(uint32_t)triangles.size() * sizeof(Triangle)},
			.name = {"TrianglesBuffer"}
		});

	triangleIndxsBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::StorageBuffer},
			.memoryAccess = {MemoryAccess::GPU},
			.size = {indicesNum * sizeof(uint32_t)},
			.name = {"TrianglesIndexBuffer"}
		});

	cfbvhNodesBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::StorageBuffer},
			.memoryAccess = {MemoryAccess::GPU},
			.size = {nodeNum * sizeof(CacheFriendlyBVHNode)},
			.name = {"CfbvhNodes"}
		});
   
	vertexBuffer->FillBuffer(verticesFinal.data(), verticesFinal.size() * sizeof(rabbitVec4f));
	trianglesBuffer->FillBuffer(triangles.data(), triangles.size() * sizeof(Triangle));
	triangleIndxsBuffer->FillBuffer(triIndices, indicesNum * sizeof(uint32_t));
	cfbvhNodesBuffer->FillBuffer(root, nodeNum * sizeof(CacheFriendlyBVHNode));
}

void Renderer::UpdateConstantBuffer()
{
	rabbitVec4f frustrumInfo{};
	frustrumInfo.x = GetNativeWidth;
	frustrumInfo.y = GetNativeHeight;
    frustrumInfo.z = MainCamera->GetNearPlane();
    frustrumInfo.w = MainCamera->GetFarPlane();

    m_StateManager->UpdateUBOElement(UBOElement::FrustrumInfo, 1, &frustrumInfo);

	rabbitVec4f frameInfo{};
	frameInfo.x = m_CurrentFrameIndex;

	m_StateManager->UpdateUBOElement(UBOElement::CurrentFrameInfo, 1, &frameInfo);
}

void Renderer::UpdateUIStateAndFSR2PreDraw()
{
	m_CurrentUIState->camera = MainCamera;
	m_CurrentUIState->deltaTime = m_CurrentDeltaTime * 1000.f; //needs to be in ms
	m_CurrentUIState->renderHeight = GetNativeHeight;
	m_CurrentUIState->renderWidth = GetNativeWidth;
	m_CurrentUIState->sharpness = 1.f;
	m_CurrentUIState->reset = hasViewProjMatrixChanged;
	m_CurrentUIState->useRcas = true;
	m_CurrentUIState->useTaa = true;

	SuperResolutionManager::instance().PreDraw(m_CurrentUIState);
}

void Renderer::ImguiProfilerWindow(std::vector<TimeStamp>& timeStamps)
{
	ImGui::Begin("GPU Profiler");

	ImGui::TextWrapped("GPU Adapter: %s", m_VulkanDevice.GetPhysicalDeviceProperties().deviceName);
	ImGui::Text("Display Resolution : %ix%i", GetUpscaledWidth, GetUpscaledHeight);
	ImGui::Text("Render Resolution : %ix%i", GetNativeWidth, GetNativeHeight);

	int fps = 1.f / m_CurrentDeltaTime;
	auto frameTime_ms = m_CurrentDeltaTime * 1000.f;

	ImGui::Text("FPS        : %d (%.2f ms)", fps, frameTime_ms);

	if (ImGui::CollapsingHeader("GPU Timings", ImGuiTreeNodeFlags_DefaultOpen))
	{

		const float unitScaling = 1.0f / 1000.0f; //turn to miliseconds

		for (uint32_t i = 0; i < timeStamps.size(); i++)
		{
			float value = timeStamps[i].microseconds * unitScaling;
			ImGui::Text("%-27s: %7.2f %s", timeStamps[i].label.c_str(), value, "ms");

			// 		if (m_UIState.bShowAverage)
			// 		{
			// 			if (m_UIState.displayedTimeStamps.size() == timeStamps.size())
			// 			{
			// 				ImGui::SameLine();
			// 				ImGui::Text("  avg: %7.2f %s", m_UIState.displayedTimeStamps[i].sum * unitScaling, pStrUnit);
			// 				ImGui::SameLine();
			// 				ImGui::Text("  min: %7.2f %s", m_UIState.displayedTimeStamps[i].minimum * unitScaling, pStrUnit);
			// 				ImGui::SameLine();
			// 				ImGui::Text("  max: %7.2f %s", m_UIState.displayedTimeStamps[i].maximum * unitScaling, pStrUnit);
			// 			}
			// 
			// 			UIState::AccumulatedTimeStamp* pAccumulatingTimeStamp = &m_UIState.accumulatingTimeStamps[i];
			// 			pAccumulatingTimeStamp->sum += timeStamps[i].m_microseconds;
			// 			pAccumulatingTimeStamp->minimum = min(pAccumulatingTimeStamp->minimum, timeStamps[i].m_microseconds);
			// 			pAccumulatingTimeStamp->maximum = max(pAccumulatingTimeStamp->maximum, timeStamps[i].m_microseconds);
			// 		}
		}
	}

	ImGui::End(); // PROFILER
}

void Renderer::RegisterTexturesToImGui()
{
	debugTextureImGuiDS = ImGui_ImplVulkan_AddTexture(debugTexture->GetSampler()->GetSampler(), debugTexture->GetView()->GetImageView(), GetVkImageLayoutFrom(ResourceState::GenericRead));
}

void Renderer::ImGuiTextureDebugger()
{
	ImGui::Begin("Texture Debugger");

	auto& textures = m_ResourceManager->GetTextures();

	if (ImGui::BeginCombo("##combo", currentTextureSelectedName.c_str())) // The second parameter is the label previewed before opening the combo.
	{
		for (auto& textureIdPair : textures)
		{
			auto currentTexture = textureIdPair.second;

			if (currentTexture == nullptr)
				continue;

			auto currentId = textureIdPair.first;
			std::string currentTextureName = currentTexture->GetName();

			bool is_selected = (currentTextureSelectedID == currentId); // You can store your selection however you want, outside or inside your objects
			if (ImGui::Selectable(currentTextureName.c_str(), is_selected))
			{
				currentTextureSelectedName = currentTextureName.c_str();
				currentTextureSelectedID = currentId;
			}
			if (is_selected)
			{
				ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
			}
		}
		ImGui::EndCombo();
	}

	VulkanTexture* currentSelectedTexture = GetTextureWithID(currentTextureSelectedID);

	if (currentSelectedTexture)
	{
		auto region = currentSelectedTexture->GetRegion();

		uint32_t arraySize = region.Subresource.ArraySize;
		bool isArray = arraySize > 1;

		bool is3D = region.Extent.Depth > 1;
		debugTextureParams.is3D = is3D;

		debugTextureParams.isArray = isArray;
		debugTextureParams.arrayCount = arraySize;

		if (isArray)
		{
			ImGui::SliderInt("Array Slice: ", &debugTextureParams.arraySlice, 0, arraySize - 1);
		}
		else if (is3D)
		{
			ImGui::SliderFloat("Depth Scale: ", &debugTextureParams.texture3DDepthScale, 0.f, 1.f);
		}

		static bool r = true, g = true, b = true, a = true;
		ImGui::Checkbox("R:", &r);
		ImGui::SameLine();
		ImGui::Checkbox("G:", &g);
		ImGui::SameLine();
		ImGui::Checkbox("B:", &b);
		ImGui::SameLine();
		ImGui::Checkbox("A:", &a);
		debugTextureParams.showR = r;
		debugTextureParams.showG= g;
		debugTextureParams.showB = b;
		debugTextureParams.showA = a;

		auto textureDebugerSize = ImGui::GetWindowSize();
		const float minTextureHeight = 230.f;
		const float textureInfoOffset = 100.f;

		float textureWidth = static_cast<float>(currentSelectedTexture->GetWidth());
		float textureHeight = static_cast<float>(currentSelectedTexture->GetHeight());

		float textureFinalHeight = textureDebugerSize.y - textureInfoOffset;
		float aspectRatio = textureWidth / textureHeight;

		float textureFinalWidth = textureFinalHeight * aspectRatio;

		ImGui::Image((void*)debugTextureImGuiDS, ImVec2(textureFinalWidth, textureFinalHeight));
	}

	ImGui::End();
}

void Renderer::BindViewport(float x, float y, float width, float height)
{
	VkViewport viewport;
	viewport.x = x;
	viewport.y = y;
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = { static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height) };

	vkCmdSetViewport(GetCurrentCommandBuffer(), 0, 1, &viewport);
	vkCmdSetScissor(GetCurrentCommandBuffer(), 0, 1, &scissor);
}

void Renderer::BindVertexData(size_t offset)
{
	VkBuffer buffers[] = { m_VertexUploadBuffer->GetBuffer() };
	VkDeviceSize offsets[] = { (VkDeviceSize)offset };
	vkCmdBindVertexBuffers(GetCurrentCommandBuffer(), 0, 1, buffers, offsets);
}

void Renderer::DrawVertices(uint64_t count)
{
	BindGraphicsPipeline();

	BindDescriptorSets();

	BeginRenderPass({ GetNativeWidth, GetNativeHeight });

	vkCmdDraw(GetCurrentCommandBuffer(), count, 1, 0, 0);

	EndRenderPass();
}

void Renderer::DrawIndicesIndirect(uint32_t count, uint32_t offset)
{
	vkCmdDrawIndexedIndirect(GetCurrentCommandBuffer(), geomDataIndirectDraw->gpuBuffer->GetBuffer(), offset * sizeof(IndexIndirectDrawData), 1, sizeof(IndexIndirectDrawData));

	geomDataIndirectDraw->localBuffer[offset].firstIndex = 0;
	geomDataIndirectDraw->localBuffer[offset].firstInstance = 0;
	geomDataIndirectDraw->localBuffer[offset].indexCount = count;
	geomDataIndirectDraw->localBuffer[offset].instanceCount = 1;
	geomDataIndirectDraw->localBuffer[offset].vertexOffset = 0;
}

void Renderer::Dispatch(uint32_t x, uint32_t y, uint32_t z)
{
	vkCmdDispatch(GetCurrentCommandBuffer(), x, y, z);
}

void Renderer::CopyImageToBuffer(VulkanTexture* texture, VulkanBuffer* buffer)
{
	VkCommandBuffer commandBuffer = m_VulkanDevice.BeginSingleTimeCommands();

	ImageRegion texRegion = texture->GetRegion();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = texRegion.Subresource.MipSlice;
	region.imageSubresource.baseArrayLayer = texRegion.Subresource.ArraySlice;
	region.imageSubresource.layerCount = texRegion.Subresource.MipSize;

	region.imageOffset = { texRegion.Offset.X, texRegion.Offset.Y, texRegion.Offset.Z };
	region.imageExtent = { texRegion.Extent.Width, texRegion.Extent.Height, 1 };

	vkCmdCopyImageToBuffer(commandBuffer, texture->GetResource()->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer->GetBuffer(), 1, &region);
	m_VulkanDevice.EndSingleTimeCommands(commandBuffer);
}

void Renderer::CopyImage(VulkanTexture* src, VulkanTexture* dst)
{
	VkCommandBuffer commandBuffer = GetCurrentCommandBuffer();

	VkImageCopy imageCopyRegion{};
	imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopyRegion.srcSubresource.layerCount = 1;
	imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopyRegion.dstSubresource.layerCount = 1;
	imageCopyRegion.extent.width = src->GetWidth();
	imageCopyRegion.extent.height = src->GetHeight();
	imageCopyRegion.extent.depth = 1;

	vkCmdCopyImage(
		commandBuffer, 
		src->GetResource()->GetImage(), 
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
		dst->GetResource()->GetImage(), 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		1, 
		&imageCopyRegion);
}

void IndexedIndirectBuffer::SubmitToGPU()
{
	gpuBuffer->FillBuffer(localBuffer, currentOffset * sizeof(IndexIndirectDrawData));
}


void IndexedIndirectBuffer::AddIndirectDrawCommand(VkCommandBuffer commandBuffer, IndexIndirectDrawData& drawData)
{
    vkCmdDrawIndexedIndirect(commandBuffer, gpuBuffer->GetBuffer(), currentOffset * sizeof(IndexIndirectDrawData), 1, sizeof(IndexIndirectDrawData));
	
	localBuffer[currentOffset] = drawData;
	currentOffset++;
}

void IndexedIndirectBuffer::Reset()
{
	currentOffset = 0;
}
