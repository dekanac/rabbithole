#include "Renderer.h"

#include "Core/Application.h"
#include "ECS/EntityManager.h"
#include "Input/InputManager.h"
#include "Model/Model.h"
#include "Render/Camera.h"
#include "Render/Converters.h"
#include "Render/PipelineManager.h"
#include "Render/RabbitPass.h"
#include "Render/ResourceStateTracking.h"
#include "Render/Shader.h"
#include "Render/SuperResolutionManager.h"
#include "Render/Window.h"
#include "Utils/utils.h"
#include "Vulkan/Include/VulkanWrapper.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <set>
#include <sstream>
#include <thread>
#include <vector>

bool Renderer::Init()
{
	m_MainCamera.Init();
	SuperResolutionManager::instance().Init(&m_VulkanDevice);
	m_ResourceManager = new ResourceManager();
	m_StateManager = new VulkanStateManager();

#ifdef RABBITHOLE_USING_IMGUI
	ImGui::CreateContext();
#endif

	InitDefaultTextures();
	LoadModels();
	LoadAndCreateShaders();
	RecreateSwapchain();

	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateCommandBuffers();

	RabbitPassManager::instance().SchedulePasses();
	RabbitPassManager::instance().DeclareResources(this);

	m_GPUTimeStamps.OnCreate(&m_VulkanDevice, m_VulkanSwapchain->GetImageCount());

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		CreateGeometryDescriptors(gltfModels, i);
	}

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

	//init acceleration structure
	ConstructBVH();
	InitLights();

	return true;
}

bool Renderer::Shutdown()
{
	VULKAN_API_CALL(vkDeviceWaitIdle(m_VulkanDevice.GetGraphicDevice()));

	gltfModels.clear();
	m_GPUTimeStamps.OnDestroy();
	delete m_ResourceManager;
	SuperResolutionManager::instance().Destroy();
	PipelineManager::instance().Destroy();
	DestroyImgui();
	delete m_StateManager;

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

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
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
}

void Renderer::CreateGeometryDescriptors(std::vector<VulkanglTFModel>& models, uint32_t imageIndex)
{
	VulkanDescriptorSetLayout descrSetLayout(&m_VulkanDevice, { GetShader("VS_GBuffer"), GetShader("FS_GBuffer") }, "GeometryDescSetLayout");

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

			VulkanDescriptorSet* descriptorSet = PipelineManager::instance().FindOrCreateDescriptorSet(m_VulkanDevice, m_DescriptorPool.get(), &descrSetLayout, { bufferDescr, cisDescr, cis2Descr, cis3Descr });

			modelMaterial.materialDescriptorSet[imageIndex] = descriptorSet;
		}
	}
}

void Renderer::InitImgui()
{
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	io.ConfigWindowsMoveFromTitleBarOnly = true;

	//this initializes imgui for GLFW
	ImGui_ImplGlfw_InitForVulkan(Window::instance().GetNativeWindowHandle(), true);

	ImGui_ImplVulkan_InitInfo init_info = {};
	m_VulkanDevice.InitImguiForVulkan(init_info);
	init_info.DescriptorPool = GET_VK_HANDLE_PTR(m_DescriptorPool);
	init_info.MinImageCount = m_VulkanSwapchain->GetImageCount();
	init_info.ImageCount = m_VulkanSwapchain->GetImageCount();

	ImGui_ImplVulkan_Init(&init_info, GET_VK_HANDLE_PTR(m_StateManager->GetRenderPass()));

	VulkanCommandBuffer tempCommandBuffer(m_VulkanDevice, "Temp Imgui Command Buffer");
	tempCommandBuffer.BeginCommandBuffer(true);
	io.Fonts->AddFontFromFileTTF("res/fonts/Cousine-Regular.ttf", 14.f);
	ImGui_ImplVulkan_CreateFontsTexture(GET_VK_HANDLE(tempCommandBuffer));
	tempCommandBuffer.EndAndSubmitCommandBuffer();

	//clear font textures from cpu data
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.08f, 0.50f, 0.72f, 1.00f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
	style.Colors[ImGuiCol_Separator] = style.Colors[ImGuiCol_Border];
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.41f, 0.42f, 0.44f, 1.00f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.29f, 0.30f, 0.31f, 0.67f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.08f, 0.09f, 0.83f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.33f, 0.34f, 0.36f, 0.83f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.23f, 0.23f, 0.24f, 1.00f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	style.Colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
	style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	style.GrabRounding = style.FrameRounding = 2.3f;
}

void Renderer::DestroyImgui()
{
	ImGui_ImplVulkan_Shutdown();
}

void Renderer::InitDefaultTextures()
{
	g_DefaultWhiteTexture = m_ResourceManager->CreateTexture(m_VulkanDevice, "res/textures/default_white.png", ROTextureCreateInfo{
			.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst},
			.format = {Format::R8G8B8A8_UNORM_SRGB},
			.name = {"Default White"}
		});
	
	g_DefaultBlackTexture = m_ResourceManager->CreateTexture(m_VulkanDevice, "res/textures/default_black.png", ROTextureCreateInfo{
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

	noise2DTexture = m_ResourceManager->CreateTexture(m_VulkanDevice, "res/textures/noise3.png", ROTextureCreateInfo{
			.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst},
			.format = {Format::B8G8R8A8_UNORM},
			.name = {"Noise2D"}
		});

	blueNoise2DTexture = m_ResourceManager->CreateTexture(m_VulkanDevice, "res/textures/noise.png", ROTextureCreateInfo{
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
	//gltfModels.emplace_back(&m_VulkanDevice, "res/meshes/separateObjects.gltf");
	gltfModels.emplace_back(&m_VulkanDevice, "res/meshes/cottage.gltf");
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

void Renderer::BeginLabel(const char* name)
{
	m_VulkanDevice.BeginLabel(GetCurrentCommandBuffer(), name);
}

void Renderer::EndLabel()
{
	m_VulkanDevice.EndLabel(GetCurrentCommandBuffer());
}

void Renderer::RecordGPUTimeStamp(const char* label)
{
	if (m_RecordGPUTimeStamps)
		m_GPUTimeStamps.GetTimeStamp(GetCurrentCommandBuffer(), label);
}

void Renderer::DrawGeometryGLTF(std::vector<VulkanglTFModel>& bucket)
{
	BindPipeline<GraphicsPipeline>();

	BeginRenderPass({ GetNativeWidth, GetNativeHeight });

	for (auto& model : bucket)
	{
		model.BindBuffers(GetCurrentCommandBuffer());

		model.Draw(GetCurrentCommandBuffer(), m_StateManager->GetPipeline()->GetPipelineLayout(), m_CurrentImageIndex, geomDataIndirectDraw);
	}

    geomDataIndirectDraw->SubmitToGPU();

	EndRenderPass();
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
	////steal input comp from camera to retrieve mouse pos
	//auto cameras = EntityManager::instance().GetAllEntitiesWithComponent<CameraComponent>();
	//auto inputComponent = cameras[0]->GetComponent<InputComponent>();
	//auto x = inputComponent->mouse_current_x;
	//auto y = inputComponent->mouse_current_y;
	//
	//if (!(x >= 0 && y >= 0 && x <= GetNativeWidth && y <= GetNativeHeight))
	//{
	//	x = 0; //TODO: fix this. not a great solution but ok
	//	y = 0;
	//}
	//
	//PushMousePos push{};
	//push.x = static_cast<uint32_t>(x);
	//push.y = static_cast<uint32_t>(y);
	////BindPushConstant(push); //TODO: fix this
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

	BeginRenderPass({ Window::instance().GetExtent().width, Window::instance().GetExtent().height });

	//if editor is activated render into ImGui image, else render full screen into swapchain
	if (!isInEditorMode)
		vkCmdDraw(GET_VK_HANDLE(GetCurrentCommandBuffer()), 3, 1, 0, 0);

#ifdef RABBITHOLE_USING_IMGUI
	//DRAW UI(imgui)
	if (m_ImguiInitialized && imguiReady)
	{
		if (isInEditorMode)
		{
			ImGui::Begin("Viewport");
			ImGui::Image(finalImageImGuiDS, GetScaledSizeWithAspectRatioKept(ImVec2(static_cast<float>(GetUpscaledWidth), static_cast<float>(GetUpscaledHeight))));
			ImGui::End();
		}

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), GET_VK_HANDLE(GetCurrentCommandBuffer()));
		
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	
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
	//disable jitter when camera is moving
	if (m_CurrentCameraState.HasViewProjMatrixChanged)
		m_CurrentCameraState.ProjMatrixJittered = camera->Projection();
	else
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
	VulkanDescriptorSet* descriptorSet = m_StateManager->FinalizeDescriptorSet(m_VulkanDevice, m_DescriptorPool.get());
	VulkanPipeline* pipeline = m_StateManager->GetPipeline();

	vkCmdBindDescriptorSets(
		GET_VK_HANDLE(GetCurrentCommandBuffer()), 
		GetVkBindPointFrom(pipeline->GetType()),
		*pipeline->GetPipelineLayout(),
		0, 
		1, GET_VK_HANDLE_PTR(descriptorSet), 
		0, 
		nullptr);
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
	auto nativeWindowHandle = Window::instance().GetNativeWindowHandle();

	int currentWidth = Window::instance().GetExtent().width;
	int currentHeight = Window::instance().GetExtent().height;

	/* handles minimization of a window ? */
	while (currentWidth == 0 || currentHeight == 0)
	{
		glfwGetFramebufferSize(nativeWindowHandle, &currentWidth, &currentHeight);
		glfwWaitEvents();
	}

	VULKAN_API_CALL(vkDeviceWaitIdle(m_VulkanDevice.GetGraphicDevice()));

	m_VulkanSwapchain = std::make_unique<VulkanSwapchain>(m_VulkanDevice, Extent2D{ static_cast<uint32_t>(currentWidth), static_cast<uint32_t>(currentHeight) });
}

void Renderer::ResourceBarrier(VulkanTexture* texture, ResourceState oldLayout, ResourceState newLayout, ResourceStage srcStage, ResourceStage dstStage, uint32_t mipLevel, uint32_t mipCount)
{
	m_VulkanDevice.ResourceBarrier(GetCurrentCommandBuffer(), texture, oldLayout, newLayout, srcStage, dstStage, mipLevel, mipCount);
}

void Renderer::ResourceBarrier(VulkanBuffer* buffer, ResourceState oldLayout, ResourceState newLayout, ResourceStage srcStage, ResourceStage dstStage)
{
	m_VulkanDevice.ResourceBarrier(GetCurrentCommandBuffer(), buffer, oldLayout, newLayout, srcStage, dstStage);
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
		
		//make imgui window pos and size same as the main window
		ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
		ImGui::SetNextWindowSize(ImVec2(static_cast<float>(Window::instance().GetExtent().width), static_cast<float>(Window::instance().GetExtent().height)));
		
		ImGui::NewFrame();

		ImGui::Begin("Main debug frame");
		ImGui::Checkbox("GPU Profiler Enabled: ", &m_RecordGPUTimeStamps);

		ImGui::End();

		ImGuiTextureDebugger();

		if (m_RecordGPUTimeStamps)
		{
			ImguiProfilerWindow(timeStamps);
		}

		imguiReady = true;
	}

	UpdateUIStateAndFSR2PreDraw();
	UpdateConstantBuffer();
	BindCameraMatrices(&m_MainCamera);
	BindUBO();

	EXECUTE_ONCE(RabbitPassManager::instance().ExecuteOneTimePasses(this));
	RabbitPassManager::instance().ExecutePasses(this);

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

void Renderer::InitLights()
{
	lights.push_back(
		LightParams{
			.position = { 70.0f, 200.0f, -100.0f },
			.radius = {1.f},
			.color = {1.f, 0.6f, 0.2f},
			.intensity = {25.f},
			.type = {LightType::LightType_Directional},
			.size = {5.f}
		});

	lights.push_back(
		LightParams{
			.position = { 7.f, 2.f, -10.0f },
			.radius = {0.f},
			.color = {1.f, 0.6f, 0.2f},
			.intensity = {1.f},
			.type = {LightType::LightType_Point},
			.size = {5.f}
		});

	lights.push_back(
		LightParams{
			.position = {-10.0f, 10.0f, -10.0f},
			.radius = {0.f},
			.color = { 0.f, 0.732f, 0.36f },
			.intensity = {0.6f},
			.type = {LightType::LightType_Point},
			.size = {0.2f}
		});

	lights.push_back(
		LightParams{
			.position = {5.0f, 20.0f, -1.0f},
			.radius = {0.f},
			.color = {1.f, 0.6f, 0.2f},
			.intensity = {1.f},
			.type = {LightType::LightType_Point},
			.size = {1.f},
		});
}

void Renderer::ConstructBVH()
{
	std::vector<Triangle> triangles;
	std::vector<rabbitVec4f> verticesFinal;

	uint32_t vertexOffset = 0;

	for (auto& model : gltfModels)
	{
		std::vector<bool> verticesMultipliedWithMatrix;

		auto modelVertexBuffer = model.GetVertexBuffer();
		auto modelIndexBuffer = model.GetIndexBuffer();

		//TODO: replace this with one Main Init Render command buffer with a lifetime only in init stages
		VulkanCommandBuffer tempCommandBuffer(m_VulkanDevice, "Temp command buffer");
		tempCommandBuffer.BeginCommandBuffer();

		VulkanBuffer stagingBuffer(m_VulkanDevice, BufferUsageFlags::TransferDst, MemoryAccess::CPU, modelVertexBuffer->GetSize(), "StagingBuffer");
		m_VulkanDevice.CopyBuffer(tempCommandBuffer, *modelVertexBuffer, stagingBuffer, modelVertexBuffer->GetSize());

		Vertex* vertexBufferCpu = (Vertex*)stagingBuffer.Map();
		uint32_t vertexCount = static_cast<uint32_t>(modelVertexBuffer->GetSize() / sizeof(Vertex));
		verticesMultipliedWithMatrix.resize(vertexCount);

		VulkanBuffer stagingBuffer2(m_VulkanDevice, BufferUsageFlags::TransferDst, MemoryAccess::CPU, modelIndexBuffer->GetSize(), "StagingBuffer");
		m_VulkanDevice.CopyBuffer(tempCommandBuffer, *modelIndexBuffer, stagingBuffer2, modelIndexBuffer->GetSize());

		tempCommandBuffer.EndAndSubmitCommandBuffer();

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

			std::string name = timeStamps[i].label;
			if (name.length() >= 24)
			{
				name = name.substr(0, 22) + "...";
			}

			ImGui::Text("%-27s: %7.2f %s", name.c_str(), value, "ms");

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
	debugTextureImGuiDS = ImGui_ImplVulkan_AddTexture(GET_VK_HANDLE_PTR(TextureDebugPass::Output->GetSampler()), GET_VK_HANDLE_PTR(TextureDebugPass::Output->GetView()), GetVkImageLayoutFrom(ResourceState::GenericRead));
	finalImageImGuiDS = ImGui_ImplVulkan_AddTexture(GET_VK_HANDLE_PTR(TonemappingPass::Output->GetSampler()), GET_VK_HANDLE_PTR(TonemappingPass::Output->GetView()), GetVkImageLayoutFrom(ResourceState::GenericRead));
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
				return a.second->GetName() < b.second->GetName();
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
		auto& debugTextureParams = TextureDebugPass::ParamsCPU;

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

		ImGui::Image((void*)debugTextureImGuiDS, GetScaledSizeWithAspectRatioKept(ImVec2(textureWidth, textureHeight)));
	}

	ImGui::End();
}

ImVec2 Renderer::GetScaledSizeWithAspectRatioKept(ImVec2 currentSize)
{
	float texWidth = currentSize.x;
	float texHeight = currentSize.y;

	float inverseAspectRatio = (texHeight / float(texWidth));

	ImVec2 currentWindowSize = ImGui::GetWindowSize();

	if (currentWindowSize.x < texWidth || currentWindowSize.y < texHeight)
	{
		
		float newTexWidth = currentWindowSize.x;
		float newTexHeight = newTexWidth * inverseAspectRatio;

		if (currentWindowSize.y < newTexHeight)
		{
			newTexWidth = currentWindowSize.y * 1.f / inverseAspectRatio;
			newTexHeight = currentWindowSize.y;
		}

		return ImVec2(newTexWidth, newTexHeight);
	}
	else
	{
		return ImVec2(texWidth, texHeight);
	}

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

void Renderer::CopyBufferToImage(VulkanBuffer* buffer, VulkanTexture* texture)
{
	m_VulkanDevice.CopyBufferToImage(GetCurrentCommandBuffer(), buffer, texture);
}

void Renderer::CopyImage(VulkanTexture* src, VulkanTexture* dst)
{
	m_VulkanDevice.CopyImage(GetCurrentCommandBuffer(), src, dst);
}

void Renderer::CopyBuffer(VulkanBuffer& src, VulkanBuffer& dst, uint64_t size /*= UINT64_MAX*/, uint64_t srcOffset /*= 0*/, uint64_t dstOffset /*= 0*/)
{
	// if size is not set, we assume that we want to copy whole src buffer
	if (size == UINT64_MAX)
	{
		size = src.GetSize();
	}

	m_VulkanDevice.CopyBuffer(GetCurrentCommandBuffer(), src, dst, size, srcOffset, dstOffset);
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

	//framebufferC
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
