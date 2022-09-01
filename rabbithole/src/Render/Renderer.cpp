#define GLFW_INCLUDE_VULKAN
#include "Renderer.h"

#include "Core/Application.h"
#include "ECS/EntityManager.h"
#include "Input/InputManager.h"
#include "Model/Model.h"
#include "Render/Camera.h"
#include "Render/Converters.h"
#include "Render/Window.h"
#include "Render/RabbitPass.h"
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
#include <format>

rabbitVec3f renderDebugOption;

void Renderer::ExecuteRabbitPass(RabbitPass& rabbitPass)
{
	m_VulkanDevice.BeginLabel(GetCurrentCommandBuffer(), rabbitPass.GetName());

	rabbitPass.Setup(this);

	rabbitPass.Render(this);
	
	if (m_RecordGPUTimeStamps)
	{
		RecordGPUTimeStamp(rabbitPass.GetName());
	}

	m_VulkanDevice.EndLabel(GetCurrentCommandBuffer());
}

bool Renderer::Init()
{
	m_MainCamera.Init();

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

	const float shadowResolutionMultiplier = 1.f;

	const uint32_t shadowResX = static_cast<uint32_t>(GetNativeWidth * shadowResolutionMultiplier);
	const uint32_t shadowResY = static_cast<uint32_t>(GetNativeHeight * shadowResolutionMultiplier);

	//GBUFFER RENDER SET
	depthStencil = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{ 
			.dimensions = {GetNativeWidth , GetNativeHeight, 1},
			.flags = {TextureFlags::DepthStencil | TextureFlags::Read | TextureFlags::TransferSrc},
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
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::Storage},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"GBuffer Normal"}
		});

	worldPositionGBuffer = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetNativeWidth , GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::Storage},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"GBuffer World Position"}
		});

	velocityGBuffer = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {GetNativeWidth , GetNativeHeight, 1},
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::Storage},
			.format = {Format::R32G32_FLOAT},
			.name = {"GBuffer Velocity"}
		});
	
	//for now max 1024 commands
	constexpr uint32_t numOfIndirectCommands = 1024;

	geomDataIndirectDraw = new IndexedIndirectBuffer();
	geomDataIndirectDraw->gpuBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::IndirectBuffer | BufferUsageFlags::TransferSrc},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(IndexIndirectDrawData) * numOfIndirectCommands},
			.name = {"GeomDataIndirectDraw"}
		});

	//TODO: do this better, brother
	geomDataIndirectDraw->localBuffer = (IndexIndirectDrawData*)malloc(sizeof(IndexIndirectDrawData) * numOfIndirectCommands);

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
			.dimensions = {shadowResX, shadowResY, 1},
			.flags = {TextureFlags::Read | TextureFlags::Storage},
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
			.flags = {TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::Storage},
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
			.flags = {TextureFlags::Read | TextureFlags::TransferSrc | TextureFlags::Storage},
			.format = {Format::R16G16B16A16_FLOAT},
			.name = {"Media Density"},
		});

	scatteringTexture = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {160, 90, 64},
			.flags = {TextureFlags::Read | TextureFlags::TransferSrc | TextureFlags::Storage},
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
	const uint32_t tileW = GetCSDispatchCount(shadowResX, 8);
	const uint32_t tileH = GetCSDispatchCount(shadowResY, 4);

	const uint32_t tileSize = tileH * tileW;

	denoiseBufferDimensions = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(DenoiseBufferDimensions)},
			.name = {"Denoise Dimensions buffer"}
		});

	denoiseShadowDataBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(DenoiseShadowData)},
			.name = {"Denoise Shadow Data Buffer"}
		});
	
	denoiseLastFrameDepth = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
		.dimensions = {shadowResX, shadowResY, 1},
		.flags = {TextureFlags::DepthStencil | TextureFlags::Read | TextureFlags::TransferDst },
		.format = {Format::D32_SFLOAT},
		.name = {"Denoise Last Frame Depth"}
		});

	for (uint32_t i = 0; i < MAX_NUM_OF_LIGHTS; i++)
	{

		denoiseShadowMaskBuffer[i] = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
				.flags = {BufferUsageFlags::StorageBuffer},
				.memoryAccess = {MemoryAccess::GPU},
				.size = {tileSize * static_cast<uint32_t>(sizeof(uint32_t))},
				.name = {std::format("Denoise Shadow Mask Buffer {} slice", i)}
			});

		denoiseTileMetadataBuffer[i] = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
				.flags = {BufferUsageFlags::StorageBuffer},
				.memoryAccess = {MemoryAccess::GPU},
				.size = {tileSize * static_cast<uint32_t>(sizeof(uint32_t))},
				.name = {std::format("Denoise Tile Metadata Buffer {} slice", i)}
			});

		//classify
		denoiseMomentsBuffer0[i] = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
				.dimensions = {shadowResX, shadowResY, 1},
				.flags = {TextureFlags::Read | TextureFlags::Storage},
				.format = {Format::R11G11B10_FLOAT},
				.name = {std::format("Denoise Moments Buffer0 {} slice", i)}
			});

		denoiseMomentsBuffer1[i] = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
				.dimensions = {shadowResX, shadowResY, 1},
				.flags = {TextureFlags::Read | TextureFlags::Storage},
				.format = {Format::R11G11B10_FLOAT},
				.name = {std::format("Denoise Moments Buffer1 {} slice", i)}
			});

		denoiseReprojectionBuffer0[i] = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
				.dimensions = {shadowResX, shadowResY, 1},
				.flags = {TextureFlags::Read | TextureFlags::Storage},
				.format = {Format::R16G16_FLOAT},
				.name = {std::format("Denoise Reprojection Buffer0 {} slice", i)}
			});

		denoiseReprojectionBuffer1[i] = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
				.dimensions = {shadowResX, shadowResY, 1},
				.flags = {TextureFlags::Read | TextureFlags::Storage},
				.format = {Format::R16G16_FLOAT},
				.name = {std::format("Denoise Reprojection Buffer1 {} slice", i)}
			});
	}

	denoiseShadowFilterDataBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::UniformBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {sizeof(DenoiseShadowFilterData)},
			.name = {"Denoise Shadow Filter Data Buffer"}
		});

	denoisedShadowOutput = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {shadowResX, shadowResY, 1},
			.flags = {TextureFlags::Read | TextureFlags::Storage},
			.format = {Format::R16G16B16A16_UNORM},
			.name = {"Denoised Shadow Output"},
			.arraySize = {MAX_NUM_OF_LIGHTS},
			.isCube = { false },
			.multisampleType = {MultisampleType::Sample_1},
			.samplerType = { SamplerType::Trilinear },
			.addressMode = { AddressMode::Clamp }
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

	m_MainCamera.Update(dt);

    DrawFrame();

	m_CurrentFrameIndex++;
	
	Clear();
}

void Renderer::DrawFrame()
{
	auto result = m_VulkanSwapchain->AcquireNextImage(&m_CurrentImageIndex);
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
	{
		LOG_ERROR("failed to acquire swap chain image!");
	}

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapchain();
		return;
	}

	RecordCommandBuffer();

	result = m_VulkanSwapchain->SubmitCommandBufferAndPresent(GetCurrentCommandBuffer(), &m_CurrentImageIndex);

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
	init_info.DescriptorPool = GET_VK_HANDLE_PTR(imguiDescriptorPool);
	init_info.MinImageCount = m_VulkanSwapchain->GetImageCount();
	init_info.ImageCount = m_VulkanSwapchain->GetImageCount();

	ImGui_ImplVulkan_Init(&init_info, GET_VK_HANDLE_PTR(m_StateManager->GetRenderPass()));

	VulkanCommandBuffer tempCommandBuffer(m_VulkanDevice, "Temp Imgui Command Buffer");
	tempCommandBuffer.BeginCommandBuffer(true);
	ImGui_ImplVulkan_CreateFontsTexture(GET_VK_HANDLE(tempCommandBuffer));
	tempCommandBuffer.EndAndSubmitCommandBuffer();

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
			.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst | TextureFlags::Storage},
			.format = {Format::B8G8R8A8_UNORM},
			.name = {"BlueNoise2D"}
		});

	noise3DLUT = m_ResourceManager->CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {256, 256, 256},
			.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::Storage},
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
	renderPassInfo.renderPass = GET_VK_HANDLE_PTR(m_StateManager->GetRenderPass());
	renderPassInfo.framebuffer = GET_VK_HANDLE_PTR(m_StateManager->GetFramebuffer());
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

	vkCmdBeginRenderPass(GET_VK_HANDLE(GetCurrentCommandBuffer()), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void Renderer::EndRenderPass()
{
	vkCmdEndRenderPass(GET_VK_HANDLE(GetCurrentCommandBuffer()));
	m_StateManager->Reset();
}

void Renderer::RecordGPUTimeStamp(const char* label)
{
	m_GPUTimeStamps.GetTimeStamp(GetCurrentCommandBuffer(), label);
}

void Renderer::DrawGeometryGLTF(std::vector<VulkanglTFModel>& bucket)
{
	BindPipeline<GraphicsPipeline>();

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

void Renderer::FillTheLightParam(LightParams& lightParam, rabbitVec3f position, rabbitVec3f color, float radius, float intensity, LightType type, float size)
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
	lightParam.size = size;
}

void Renderer::DrawFullScreenQuad()
{
	BindPipeline<GraphicsPipeline>();

	Extent2D renderExtent{};
	renderExtent.width = m_StateManager->GetFramebufferExtent().width;
	renderExtent.height = m_StateManager->GetFramebufferExtent().height;

	BeginRenderPass(renderExtent);

	vkCmdDraw(GET_VK_HANDLE(GetCurrentCommandBuffer()), 3, 1, 0, 0);

	EndRenderPass();
}

void Renderer::BindPushConstInternal()
{
	if (m_StateManager->ShouldBindPushConst())
	{
		VkShaderStageFlagBits shaderStage = (m_StateManager->GetPipeline()->GetType() == PipelineType::Graphics) ? VkShaderStageFlagBits(VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT) : (VK_SHADER_STAGE_COMPUTE_BIT);
		//TODO: what the hell is going on here
		vkCmdPushConstants(GET_VK_HANDLE(GetCurrentCommandBuffer()),
			*(m_StateManager->GetPipeline()->GetPipelineLayout()),
			shaderStage,
			0,
			m_StateManager->GetPushConst().size,
			&m_StateManager->GetPushConst().data);

		m_StateManager->SetShouldBindPushConst(false);
	}
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
	push.x = static_cast<uint32_t>(x);
	push.y = static_cast<uint32_t>(y);
	//BindPushConstant(push); //TODO: fix this
}

void Renderer::CopyToSwapChain()
{
	BindPipeline<GraphicsPipeline>();

#ifdef RABBITHOLE_USING_IMGUI
	if (!m_ImguiInitialized)
	{
		InitImgui();
		RegisterTexturesToImGui();
		m_ImguiInitialized = true;
	}
#endif

	BeginRenderPass({ GetUpscaledWidth, GetUpscaledHeight });

	//COPY TO SWAPCHAIN
	vkCmdDraw(GET_VK_HANDLE(GetCurrentCommandBuffer()), 3, 1, 0, 0);

#ifdef RABBITHOLE_USING_IMGUI
	//DRAW UI(imgui)
	if (m_ImguiInitialized && imguiReady)
	{
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), GET_VK_HANDLE(GetCurrentCommandBuffer()));
	}
#endif

	EndRenderPass();
}

void Renderer::BindCameraMatrices(Camera* camera)
{	
	auto projection = camera->Projection();
	auto view = camera->View();

	m_StateManager->UpdateUBOElement(UBOElement::PrevViewProjMatrix, 4, &m_CurrentCameraState.PrevViewProjMatrix);

	m_CurrentCameraState.ViewMatrix = view;
	m_StateManager->UpdateUBOElement(UBOElement::ViewMatrix, 4, &m_CurrentCameraState.ViewMatrix);

	m_CurrentCameraState.ProjectionMatrix = projection;
	m_StateManager->UpdateUBOElement(UBOElement::ProjectionMatrix, 4, &m_CurrentCameraState.ProjectionMatrix);

	//todo: double check this, for now I use jittered matrix in VS_Gbuffer FS_SSAO and VS_Skybox
	m_CurrentCameraState.ProjMatrixJittered = camera->ProjectionJittered();
	m_StateManager->UpdateUBOElement(UBOElement::ProjectionMatrixJittered, 4, &m_CurrentCameraState.ProjMatrixJittered);

	m_CurrentCameraState.ViewProjMatrix = projection * view;
	m_StateManager->UpdateUBOElement(UBOElement::ViewProjMatrix, 4, &m_CurrentCameraState.ViewProjMatrix);

	m_CurrentCameraState.CameraPosition = camera->GetPosition();
	m_StateManager->UpdateUBOElement(UBOElement::CameraPosition, 1, &m_CurrentCameraState.CameraPosition);
	
	m_CurrentCameraState.ViewInverseMatrix = glm::inverse(view);
	m_StateManager->UpdateUBOElement(UBOElement::ViewInverse, 4, &m_CurrentCameraState.ViewInverseMatrix);

	m_CurrentCameraState.ProjectionInverseMatrix = glm::inverse(projection);
	m_StateManager->UpdateUBOElement(UBOElement::ProjInverse, 4, &m_CurrentCameraState.ProjectionInverseMatrix);

	m_CurrentCameraState.ViewProjInverseMatrix = m_CurrentCameraState.ViewInverseMatrix * m_CurrentCameraState.ProjectionInverseMatrix;
	m_StateManager->UpdateUBOElement(UBOElement::ViewProjInverse, 4, &m_CurrentCameraState.ViewProjInverseMatrix);

	float width = projection[0][0];
	float height = projection[1][1];

	m_CurrentCameraState.EyeXAxis = m_CurrentCameraState.ViewInverseMatrix * rabbitVec4f(-1.0 / width, 0, 0, 0);
	m_CurrentCameraState.EyeYAxis = m_CurrentCameraState.ViewInverseMatrix * rabbitVec4f(0, -1.0 / height, 0, 0);
	m_CurrentCameraState.EyeZAxis = m_CurrentCameraState.ViewInverseMatrix * rabbitVec4f(0, 0, 1.f, 0);

	m_StateManager->UpdateUBOElement(UBOElement::EyeXAxis, 1, &m_CurrentCameraState.EyeXAxis);
	m_StateManager->UpdateUBOElement(UBOElement::EyeYAxis, 1, &m_CurrentCameraState.EyeYAxis);
	m_StateManager->UpdateUBOElement(UBOElement::EyeZAxis, 1, &m_CurrentCameraState.EyeZAxis);

	if (m_CurrentCameraState.ViewProjMatrix == m_CurrentCameraState.PrevViewProjMatrix)
		m_CurrentCameraState.HasViewProjMatrixChanged = false;
	else
		m_CurrentCameraState.HasViewProjMatrixChanged = true;

	m_CurrentCameraState.PrevViewProjMatrix = m_CurrentCameraState.ViewProjMatrix;
	m_CurrentCameraState.PrevViewMatrix = m_CurrentCameraState.ViewMatrix;
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

	vkCmdBindDescriptorSets(GET_VK_HANDLE(GetCurrentCommandBuffer()), bindPoint, *pipeline->GetPipelineLayout(), 0, 1, GET_VK_HANDLE_PTR(descriptorSet), 0, nullptr);
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

					auto shaderCode = Utils::ReadFile(filePath);
					
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
	m_MainRenderCommandBuffers.resize(m_VulkanSwapchain->GetImageCount());

	for (uint32_t i = 0; i < m_VulkanSwapchain->GetImageCount(); i++)
	{
		m_MainRenderCommandBuffers[i] = std::make_unique<VulkanCommandBuffer>(m_VulkanDevice, "Main Render Command Buffer");
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

void Renderer::ResourceBarrier(VulkanTexture* texture, ResourceState oldLayout, ResourceState newLayout, ResourceStage srcStage, ResourceStage dstStage, uint32_t mipLevel)
{
	m_VulkanDevice.ResourceBarrier(GetCurrentCommandBuffer(), texture, oldLayout, newLayout, srcStage, dstStage, mipLevel);
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
void Renderer::RecordCommandBuffer()
{
	GetCurrentCommandBuffer().BeginCommandBuffer();

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
	SSAOPass ssao{};
	SSAOBlurPass ssaoBlur{};
	RTShadowsPass RTShadows{};
	ShadowDenoisePrePass shadowDenoisePrepass{};
	ShadowDenoiseTileClassificationPass shadowDenoiseTileClassification{};
	ShadowDenoiseFilterPass shadowDenoiseFilter{};
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
	BindCameraMatrices(&m_MainCamera);

	//replace with call once impl
	if (!init3dnoise)
	{
		ExecuteRabbitPass(noise3DLUTGeneration);
		init3dnoise = true;
	}

	ExecuteRabbitPass(gbuffer);
	ExecuteRabbitPass(ssao);
	ExecuteRabbitPass(ssaoBlur);
	ExecuteRabbitPass(RTShadows);
	ExecuteRabbitPass(shadowDenoisePrepass);
	ExecuteRabbitPass(shadowDenoiseTileClassification);
	ExecuteRabbitPass(shadowDenoiseFilter);
	ExecuteRabbitPass(volumetric);
	ExecuteRabbitPass(computeScattering);
	ExecuteRabbitPass(skybox);
	ExecuteRabbitPass(lighting);
	ExecuteRabbitPass(applyVolumerticFog);
    ExecuteRabbitPass(textureDebug);

#ifdef USE_RABBITHOLE_TOOLS
	if (m_RenderOutlinedEntity)
	{
		ExecuteRabbitPass(outlineEntity);
	}
#endif

	ExecuteRabbitPass(fsr2);

	ExecuteRabbitPass(tonemapping);

	ExecuteRabbitPass(copyToSwapchain);

	if (m_RecordGPUTimeStamps)
	{
		m_GPUTimeStamps.OnEndFrame();
	}

	GetCurrentCommandBuffer().EndCommandBuffer();
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
	uboPoolSize.Count = 400;
	uboPoolSize.Type = DescriptorType::UniformBuffer;

	VulkanDescriptorPoolSize samImgPoolSize{};
	samImgPoolSize.Count = 400;
	samImgPoolSize.Type = DescriptorType::SampledImage;

	VulkanDescriptorPoolSize sPoolSize{};
	sPoolSize.Count = 400;
	sPoolSize.Type = DescriptorType::Sampler;

	VulkanDescriptorPoolSize cisPoolSize{};
	cisPoolSize.Count = 400;
	cisPoolSize.Type = DescriptorType::CombinedSampler;

	VulkanDescriptorPoolSize siPoolSize{};
	siPoolSize.Count = 400;
	siPoolSize.Type = DescriptorType::StorageImage;

	VulkanDescriptorPoolSize sbPoolSize{};
	sbPoolSize.Count = 400;
	sbPoolSize.Type = DescriptorType::StorageBuffer;

	VulkanDescriptorPoolInfo vulkanDescriptorPoolInfo{};

	vulkanDescriptorPoolInfo.DescriptorSizes = { uboPoolSize, cisPoolSize, siPoolSize, sbPoolSize, samImgPoolSize, sPoolSize };

	vulkanDescriptorPoolInfo.MaxSets = 2000;

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
		float scale = static_cast<float>(i) / 64.f;

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
	ssaoParams.resWidth = static_cast<float>(GetNativeWidth);
	ssaoParams.resHeight = static_cast<float>(GetNativeHeight);
	ssaoParams.power = 1.75f;
	ssaoParams.kernelSize = 48;
	ssaoParams.ssaoOn = true;
}

void Renderer::InitLights()
{
	FillTheLightParam(lightParams[0], { 70.0f, 300.75f, -20.75f }, { 1.f, 0.6f, 0.2f }, 25.f, 1.f, LightType_Directional, 5.f);
	FillTheLightParam(lightParams[1], { 5.0f, 20.0f, -1.0f }, { 0.95f, 0.732f, 0.36f }, 0.f, 1.f, LightType_Point, 0.5f);
	FillTheLightParam(lightParams[2], { -10.0f, 10.0f, -10.0f }, { 1.f, 0.6f, 0.2f }, 0.f, 1.f, LightType_Point, 0.5f);
	FillTheLightParam(lightParams[3], { 5.f, 5.0f, 4.f }, { 1.f, 1.0f, 1.0f }, 0.f, 1.f, LightType_Point, 1.f);
}

void Renderer::InitMeshDataForCompute()
{
	std::vector<Triangle> triangles;
	std::vector<rabbitVec4f> verticesFinal;

	uint32_t vertexOffset = 0;

	for (auto& model : gltfModels)
	{
		std::vector<bool> verticesMultipliedWithMatrix;

		auto modelVertexBuffer = model.GetVertexBuffer();
		auto modelIndexBuffer = model.GetIndexBuffer();

		VulkanBuffer stagingBuffer(m_VulkanDevice, BufferUsageFlags::TransferDst, MemoryAccess::CPU, modelVertexBuffer->GetSize(), "StagingBuffer");
		m_VulkanDevice.CopyBuffer(*modelVertexBuffer, stagingBuffer, modelVertexBuffer->GetSize());

		Vertex* vertexBufferCpu = (Vertex*)stagingBuffer.Map();
		uint32_t vertexCount = static_cast<uint32_t>(modelVertexBuffer->GetSize() / sizeof(Vertex));
		verticesMultipliedWithMatrix.resize(vertexCount);

		VulkanBuffer stagingBuffer2(m_VulkanDevice, BufferUsageFlags::TransferDst, MemoryAccess::CPU, modelIndexBuffer->GetSize(), "StagingBuffer");
		m_VulkanDevice.CopyBuffer(*modelIndexBuffer, stagingBuffer2, modelIndexBuffer->GetSize());

		uint32_t* indexBufferCpu = (uint32_t*)stagingBuffer2.Map();
		uint32_t indexCount = static_cast<uint32_t>(modelIndexBuffer->GetSize() / sizeof(uint32_t));
		
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
				for (uint32_t i = primitive.firstIndex; i < primitive.firstIndex + primitive.indexCount; i++)
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

		for (uint32_t k = 0; k < vertexCount; k++)
		{
			rabbitVec4f position = rabbitVec4f{ vertexBufferCpu[k].position, 1.f };
			verticesFinal.push_back(position);
		}

		for (uint32_t j = 0; j < indexCount; j += 3)
		{
			Triangle tri;
			tri.indices[0] = vertexOffset + indexBufferCpu[j];
			tri.indices[1] = vertexOffset + indexBufferCpu[j + 1];
			tri.indices[2] = vertexOffset + indexBufferCpu[j + 2];

			triangles.push_back(tri);
		}

		vertexOffset += vertexCount;
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
			.size = {static_cast<uint32_t>(verticesFinal.size() * sizeof(rabbitVec4f))},
			.name = {"VertexBuffer"}
		});

	trianglesBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::StorageBuffer},
			.memoryAccess = {MemoryAccess::GPU},
			.size = {static_cast<uint32_t>(triangles.size() * sizeof(Triangle))},
			.name = {"TrianglesBuffer"}
		});

	triangleIndxsBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::StorageBuffer},
			.memoryAccess = {MemoryAccess::GPU},
			.size = {static_cast<uint32_t>(indicesNum * sizeof(uint32_t))},
			.name = {"TrianglesIndexBuffer"}
		});

	cfbvhNodesBuffer = m_ResourceManager->CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::StorageBuffer},
			.memoryAccess = {MemoryAccess::GPU},
			.size = {static_cast<uint32_t>(nodeNum * sizeof(CacheFriendlyBVHNode))},
			.name = {"CfbvhNodes"}
		});
   
	vertexBuffer->FillBuffer(verticesFinal.data(), static_cast<uint32_t>(verticesFinal.size()) * sizeof(rabbitVec4f));
	trianglesBuffer->FillBuffer(triangles.data(), static_cast<uint32_t>(triangles.size()) * sizeof(Triangle));
	triangleIndxsBuffer->FillBuffer(triIndices, indicesNum * sizeof(uint32_t));
	cfbvhNodesBuffer->FillBuffer(root, nodeNum * sizeof(CacheFriendlyBVHNode));
}

void Renderer::UpdateConstantBuffer()
{
	rabbitVec4f frustrumInfo{};
	frustrumInfo.x = static_cast<float>(GetNativeWidth);
	frustrumInfo.y = static_cast<float>(GetNativeHeight);
    frustrumInfo.z = m_MainCamera.GetNearPlane();
    frustrumInfo.w = m_MainCamera.GetFarPlane();

    m_StateManager->UpdateUBOElement(UBOElement::FrustrumInfo, 1, &frustrumInfo);

	rabbitVec4f frameInfo{};
	frameInfo.x = static_cast<float>(m_CurrentFrameIndex);

	m_StateManager->UpdateUBOElement(UBOElement::CurrentFrameInfo, 1, &frameInfo);
}

void Renderer::UpdateUIStateAndFSR2PreDraw()
{
	m_CurrentUIState.camera = &m_MainCamera;
	m_CurrentUIState.deltaTime = m_CurrentDeltaTime * 1000.f; //needs to be in ms
	m_CurrentUIState.renderHeight = static_cast<float>(GetNativeHeight);
	m_CurrentUIState.renderWidth = static_cast<float>(GetNativeWidth);
	m_CurrentUIState.sharpness = 1.f;
	m_CurrentUIState.reset = m_CurrentCameraState.HasViewProjMatrixChanged;
	m_CurrentUIState.useRcas = true;
	m_CurrentUIState.useTaa = true;

	SuperResolutionManager::instance().PreDraw(&m_CurrentUIState);
}

void Renderer::ImguiProfilerWindow(std::vector<TimeStamp>& timeStamps)
{
	ImGui::Begin("GPU Profiler");

	ImGui::TextWrapped("GPU Adapter: %s", m_VulkanDevice.GetPhysicalDeviceProperties().deviceName);
	ImGui::Text("Display Resolution : %ix%i", GetUpscaledWidth, GetUpscaledHeight);
	ImGui::Text("Render Resolution : %ix%i", GetNativeWidth, GetNativeHeight);

	uint32_t fps = static_cast<uint32_t>(1.f / m_CurrentDeltaTime);
	float frameTime_ms = m_CurrentDeltaTime * 1000.f;
	float numOfTriangles = static_cast<float>(trianglesBuffer->GetSize()) / static_cast<float>(sizeof(Triangle)) / 1000.f;

	ImGui::Text("FPS        : %d (%.2f ms)", fps, frameTime_ms);
	ImGui::Text("Num of triangles   : %.2fk", numOfTriangles);


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
	debugTextureImGuiDS = ImGui_ImplVulkan_AddTexture(GET_VK_HANDLE_PTR(debugTexture->GetSampler()), GET_VK_HANDLE_PTR(debugTexture->GetView()), GetVkImageLayoutFrom(ResourceState::GenericRead));
}

void Renderer::ImGuiTextureDebugger()
{
	ImGui::Begin("Texture Debugger");

	auto& texturesMap = m_ResourceManager->GetTextures();
	std::vector<std::pair<uint32_t, VulkanTexture*>> textures(texturesMap.begin(), texturesMap.end());
	std::sort(
		textures.begin(), 
		textures.end(), 
		[](const std::pair<uint32_t, VulkanTexture*>& a, const std::pair<uint32_t, VulkanTexture*>& b) 
		{
			if (a.second != nullptr && b.second != nullptr)
				return a.second->GetName() <= b.second->GetName();
			else
				return false;
		}
	);

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

	vkCmdSetViewport(GET_VK_HANDLE(GetCurrentCommandBuffer()), 0, 1, &viewport);
	vkCmdSetScissor(GET_VK_HANDLE(GetCurrentCommandBuffer()), 0, 1, &scissor);
}

void Renderer::BindVertexData(size_t offset)
{
	VkBuffer buffers[] = { GET_VK_HANDLE_PTR(m_VertexUploadBuffer) };
	VkDeviceSize offsets[] = { (VkDeviceSize)offset };
	vkCmdBindVertexBuffers(GET_VK_HANDLE(GetCurrentCommandBuffer()), 0, 1, buffers, offsets);
}

void Renderer::DrawVertices(uint32_t count)
{
	BindPipeline<GraphicsPipeline>();

	BeginRenderPass({ GetNativeWidth, GetNativeHeight });

	vkCmdDraw(GET_VK_HANDLE(GetCurrentCommandBuffer()), count, 1, 0, 0);

	EndRenderPass();
}

void Renderer::Dispatch(uint32_t x, uint32_t y, uint32_t z)
{
	BindPipeline<ComputePipeline>();

	vkCmdDispatch(GET_VK_HANDLE(GetCurrentCommandBuffer()), x, y, z);
}

void Renderer::CopyImageToBuffer(VulkanTexture* texture, VulkanBuffer* buffer)
{
	m_VulkanDevice.CopyImageToBuffer(GetCurrentCommandBuffer(), texture, buffer);
}

void Renderer::CopyImage(VulkanTexture* src, VulkanTexture* dst)
{
	m_VulkanDevice.CopyImage(GetCurrentCommandBuffer(), src, dst);
}

void IndexedIndirectBuffer::SubmitToGPU()
{
	gpuBuffer->FillBuffer(localBuffer, currentOffset * sizeof(IndexIndirectDrawData));
}


void IndexedIndirectBuffer::AddIndirectDrawCommand(VulkanCommandBuffer& commandBuffer, IndexIndirectDrawData& drawData)
{
    vkCmdDrawIndexedIndirect(GET_VK_HANDLE(commandBuffer), GET_VK_HANDLE_PTR(gpuBuffer), currentOffset * sizeof(IndexIndirectDrawData), 1, sizeof(IndexIndirectDrawData));
	
	localBuffer[currentOffset] = drawData;
	currentOffset++;
}

void IndexedIndirectBuffer::Reset()
{
	currentOffset = 0;
}

template<>
void Renderer::BindPipeline<GraphicsPipeline>()
{
	RSTManager.CommitBarriers();

	//renderpass
	auto& attachments = m_StateManager->GetRenderTargets();
	auto depthStencil = m_StateManager->GetDepthStencil();
	auto renderPassInfo = m_StateManager->GetRenderPassInfo();

	VulkanRenderPass* renderpass =
		m_StateManager->GetRenderPassDirty()
		? PipelineManager::instance().FindOrCreateRenderPass(m_VulkanDevice, attachments, depthStencil, *renderPassInfo)
		: m_StateManager->GetRenderPass();

	m_StateManager->SetRenderPass(renderpass);

	//framebuffer
	VulkanFramebufferInfo framebufferInfo{};
	framebufferInfo.width = m_StateManager->GetFramebufferExtent().width;
	framebufferInfo.height = m_StateManager->GetFramebufferExtent().height;

	VulkanFramebuffer* framebuffer =
		m_StateManager->GetFramebufferDirty()
		? PipelineManager::instance().FindOrCreateFramebuffer(m_VulkanDevice, attachments, depthStencil, renderpass, framebufferInfo)
		: m_StateManager->GetFramebuffer();

	m_StateManager->SetFramebuffer(framebuffer);

	//pipeline
	auto pipelineInfo = m_StateManager->GetPipelineInfo();

	pipelineInfo->renderPass = m_StateManager->GetRenderPass();
	auto pipeline =
		m_StateManager->GetPipelineDirty()
		? PipelineManager::instance().FindOrCreateGraphicsPipeline(m_VulkanDevice, *pipelineInfo)
		: m_StateManager->GetPipeline();

	m_StateManager->SetPipeline(pipeline);

	pipeline->Bind(GetCurrentCommandBuffer());

	BindDescriptorSets();

	BindPushConstInternal();
}

template<>
void Renderer::BindPipeline<ComputePipeline>()
{
	RSTManager.CommitBarriers();

	auto pipelineInfo = m_StateManager->GetPipelineInfo();

	VulkanPipeline* computePipeline = m_StateManager->GetPipelineDirty()
		? PipelineManager::instance().FindOrCreateComputePipeline(m_VulkanDevice, *pipelineInfo)
		: m_StateManager->GetPipeline();

	m_StateManager->SetPipeline(computePipeline);

	computePipeline->Bind(GetCurrentCommandBuffer());

	BindDescriptorSets();

	BindPushConstInternal();
}
