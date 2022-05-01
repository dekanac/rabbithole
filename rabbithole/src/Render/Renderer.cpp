#define GLFW_INCLUDE_VULKAN

#include "Renderer.h"
#include "Render/Camera.h"
#include "Render/Window.h"
#include "Render/RenderPass.h"
#include "Vulkan/Shader.h"
#include "Input/InputManager.h"
#include "ECS/EntityManager.h"
#include "Core/Application.h"
#include "Vulkan/Include/VulkanWrapper.h"
#include "SuperResolutionManager.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <vector>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <cmath>
#include <iostream>
#include <fstream>
#include <random>
#include <sstream>
#include <set>

#include "Model/ModelLoading.h"
#include "stb_image/stb_image.h"

rabbitVec3f renderDebugOption;

#define GBUFFER_VERTEX_FILE_PATH			"res/shaders/VS_GBuffer.spv"
#define GBUFFER_FRAGMENT_FILE_PATH			"res/shaders/FS_GBuffer.spv"
#define PASSTHROUGH_VERTEX_FILE_PATH		"res/shaders/VS_PassThrough.spv"
#define PASSTHROUGH_FRAGMENT_FILE_PATH		"res/shaders/FS_PassThrough.spv"
#define PBR_FRAGMENT_FILE_PATH				"res/shaders/FS_PBR.spv"
#define OUTLINE_ENTITY_FRAGMENT_FILE_PATH	"res/shaders/FS_OutlineEntity.spv"
#define SKYBOX_VERTEX_FILE_PATH				"res/shaders/VS_Skybox.spv"
#define SKYBOX_FRAGMENT_FILE_PATH			"res/shaders/FS_Skybox.spv"
#define SSAO_FRAGMENT_FILE_PATH				"res/shaders/FS_SSAO.spv"
#define SSAOBLUR_FRAGMENT_FILE_PATH			"res/shaders/FS_SSAOBlur.spv"
#define RAY_TRACING_COMPUTE_FILE_PATH		"res/shaders/CS_RayTracingShadows.spv"
#define SIMPLE_GEOMETRY_FRAGMENT_PATH		"res/shaders/FS_SimpleGeometry.spv"
#define SIMPLE_GEOMETRY_VERTEX_PATH			"res/shaders/VS_SimpleGeometry.spv"
#define FSREASU_COMPUTE_FILE_PATH			"res/shaders/CS_FSR_EASU.spv"
#define FSRRCAS_COMPUTE_FILE_PATH			"res/shaders/CS_FSR_RCAS.spv"
#define TAA_COMPUTE_FILE_PATH				"res/shaders/CS_TAA.spv"
#define TAA_SHARPENER_COMPUTE_FILE_PATH		"res/shaders/CS_TAASharpener.spv"

void Renderer::ExecuteRenderPass(RenderPass& renderpass)
{
	m_VulkanDevice.BeginLabel(GetCurrentCommandBuffer(), renderpass.GetName());

	renderpass.DeclareResources(this);
	renderpass.Setup(this);
	renderpass.Render(this);

	m_VulkanDevice.EndLabel(GetCurrentCommandBuffer());
}

void Renderer::AddSimpleMesh(SimpleMeshType type, rabbitVec3f position, float size, rabbitVec3f rotation)
{
	RabbitModel* model =nullptr;

	if (type == SimpleMeshType::BoxMesh)
	{
		model = new RabbitModel(m_VulkanDevice, defaultBoxModel->pObjects[0]);
	}
	else if (type == SimpleMeshType::SphereMesh)
	{
		model = new RabbitModel(m_VulkanDevice, defaultSphereModel->pObjects[0]);
	}

	auto mesh = model->GetMesh();
	mesh.position = position;
	mesh.rotation = rotation;
	mesh.scale = { size, size, size };

	model->SetMesh(mesh);
	rabbitmodels.push_back(model);
}

bool Renderer::Init()
{
	MainCamera = new Camera();
	MainCamera->Init();
	testEntity = new Entity();
	testEntity->AddComponent<InputComponent>();

	SuperResolutionManager::instance().Init();

	EntityManager::instance().AddEntity(testEntity);

#ifdef RABBITHOLE_USING_IMGUI
	ImGui::CreateContext();
#endif

	InitDefaultTextures(&m_VulkanDevice);
	m_StateManager = new VulkanStateManager();

	renderDebugOption.x = 0.f; //we use vector4f as a default UBO element

	loadModels();
	LoadAndCreateShaders();
	recreateSwapchain();
	createCommandBuffers();

	CreateGeometryDescriptors();
	//for now max 1024 commands
	depthStencil = new VulkanTexture(&m_VulkanDevice, GetNativeWidth, GetNativeHeight, TextureFlags::DepthStencil | TextureFlags::Read, Format::D32_SFLOAT, "swapchainDepthstencil");

	geomDataIndirectDraw = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::IndirectBuffer | BufferUsageFlags::TransferSrc, MemoryAccess::CPU2GPU, sizeof(IndexIndirectDrawData) * 1024);
	indexedDataBuffer = (IndexIndirectDrawData*)malloc(sizeof(IndexIndirectDrawData) * 1024);

	albedoGBuffer = new VulkanTexture(&m_VulkanDevice, GetNativeWidth, GetNativeHeight, TextureFlags::RenderTarget | TextureFlags::Read, Format::B8G8R8A8_UNORM, "albedo");
	normalGBuffer = new VulkanTexture(&m_VulkanDevice, GetNativeWidth, GetNativeHeight, TextureFlags::RenderTarget | TextureFlags::Read, Format::R16G16B16A16_FLOAT, "normal");
	worldPositionGBuffer = new VulkanTexture(&m_VulkanDevice, GetNativeWidth, GetNativeHeight, TextureFlags::RenderTarget | TextureFlags::Read, Format::R16G16B16A16_FLOAT, "wordlPosition");
	velocityGBuffer = new VulkanTexture(&m_VulkanDevice, GetNativeWidth, GetNativeHeight, TextureFlags::RenderTarget | TextureFlags::Read, Format::R32G32_FLOAT, "velocity");
	DebugTextureRT = new VulkanTexture(&m_VulkanDevice, GetNativeWidth, GetNativeHeight, TextureFlags::RenderTarget, Format::R16G16B16A16_FLOAT, "debug");
	entityHelper = new VulkanTexture(&m_VulkanDevice, GetNativeWidth, GetNativeHeight, TextureFlags::RenderTarget | TextureFlags::LinearTiling | TextureFlags::TransferSrc, Format::R32_UINT, "entityHelper");
	entityHelperBuffer = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::StorageBuffer, MemoryAccess::CPU, GetNativeWidth * GetNativeHeight * 4);

	TAAOutput = new VulkanTexture(&m_VulkanDevice, GetNativeWidth, GetNativeHeight, TextureFlags::RenderTarget | TextureFlags::Read, Format::R16G16B16A16_FLOAT, "TAA");
	historyBuffer = new VulkanTexture(&m_VulkanDevice, GetNativeWidth, GetNativeHeight, TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::TransferDst, Format::R16G16B16A16_FLOAT, "historyBuffer");

	shadowMap = new VulkanTexture(&m_VulkanDevice, GetNativeWidth, GetNativeHeight, TextureFlags::Read, Format::R8_UNORM, "shadowMap");
	
	InitMeshDataForCompute();

	lightingMain = new VulkanTexture(&m_VulkanDevice, GetNativeWidth, GetNativeHeight, TextureFlags::RenderTarget | TextureFlags::Read | TextureFlags::TransferSrc, Format::R16G16B16A16_FLOAT, "lightingMain");
	InitLights();

	//init fsr
	FSRParams fsrParams{};
	SuperResolutionManager::instance().CalculateFSRParamsEASU(fsrParams);
	SuperResolutionManager::instance().CalculateFSRParamsRCAS(fsrParams);
	//fsrParams.Sample[0] = 1;
	fsrParamsBuffer = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::UniformBuffer, MemoryAccess::CPU2GPU, sizeof(FSRParams));
	fsrIntermediateRes = new VulkanTexture(&m_VulkanDevice, GetUpscaledWidth, GetUpscaledHeight, TextureFlags::RenderTarget | TextureFlags::Read, Format::R16G16B16A16_FLOAT, "fsrIntermediate");
	fsrOutputTexture = new VulkanTexture(&m_VulkanDevice, GetUpscaledWidth, GetUpscaledHeight, TextureFlags::RenderTarget | TextureFlags::Read, Format::R16G16B16A16_FLOAT, "fsrOutput");
	fsrParamsBuffer->FillBuffer(&fsrParams, sizeof(FSRParams));

	InitSSAO();

	InitTextures();

	return true;
}

bool Renderer::Shutdown()
{
	delete MainCamera;
	//delete m_StateManager;
    return true;
}

void Renderer::Clear() const
{
    
}

void Renderer::Draw(float dt)
{
	MainCamera->Update(dt);
    DrawFrame();
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
		recreateSwapchain();
		return;
	}

	RecordCommandBuffer(imageIndex);

	result = m_VulkanSwapchain->SubmitCommandBuffers(&m_CommandBuffers[imageIndex], &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
	{
		m_FramebufferResized = false;
		recreateSwapchain();
	}

	if (result != VK_SUCCESS) 
	{
		LOG_ERROR("failed to present swap chain image!");
	}
}

void Renderer::CreateGeometryDescriptors()
{
	VulkanDescriptorSetLayout* descrSetLayout = new VulkanDescriptorSetLayout(&m_VulkanDevice, { m_Shaders["VS_GBuffer"], m_Shaders["FS_GBuffer"]}, "GeometryDescSetLayout");

	VulkanDescriptorInfo descriptorinfo{};
	descriptorinfo.Type = DescriptorType::UniformBuffer;
	descriptorinfo.Binding = 0;
	descriptorinfo.buffer = m_MainConstBuffer;

	VulkanDescriptor* bufferDescr = new VulkanDescriptor(descriptorinfo);

	for (size_t i = 0; i < rabbitmodels.size(); i++)
	{
		//albedo
		VulkanDescriptorInfo descriptorinfo2{};
		descriptorinfo2.Type = DescriptorType::CombinedSampler;
		descriptorinfo2.Binding = 1;

		CombinedImageSampler* imagesampler = new CombinedImageSampler();
		imagesampler->ImageSampler = rabbitmodels[i]->GetAlbedoTexture()->GetSampler();
		imagesampler->ImageView = rabbitmodels[i]->GetAlbedoTexture()->GetView();

		descriptorinfo2.combinedImageSampler = imagesampler;
		
		VulkanDescriptor* cisDescr = new VulkanDescriptor(descriptorinfo2);

		//normal
		VulkanDescriptorInfo descriptorinfo3{};
		descriptorinfo3.Type = DescriptorType::CombinedSampler;
		descriptorinfo3.Binding = 2;

		CombinedImageSampler* imagesampler2 = new CombinedImageSampler();
		imagesampler2->ImageSampler = rabbitmodels[i]->GetNormalTexture()->GetSampler();
		imagesampler2->ImageView = rabbitmodels[i]->GetNormalTexture()->GetView();

		descriptorinfo3.combinedImageSampler = imagesampler2;

		VulkanDescriptor* cis2Descr = new VulkanDescriptor(descriptorinfo3);

		//roughness
		VulkanDescriptorInfo descriptorinfo4{};
		descriptorinfo4.Type = DescriptorType::CombinedSampler;
		descriptorinfo4.Binding = 3;

		CombinedImageSampler* imagesampler3 = new CombinedImageSampler();
		imagesampler3->ImageSampler = rabbitmodels[i]->GetRoughnessTexture()->GetSampler();
		imagesampler3->ImageView = rabbitmodels[i]->GetRoughnessTexture()->GetView();

		descriptorinfo4.combinedImageSampler = imagesampler3;

		VulkanDescriptor* cis3Descr = new VulkanDescriptor(descriptorinfo4);

		VulkanDescriptorInfo descriptorinfo5{};
		descriptorinfo5.Type = DescriptorType::CombinedSampler;
		descriptorinfo5.Binding = 4;

		CombinedImageSampler* imagesampler4 = new CombinedImageSampler();
		imagesampler4->ImageSampler = rabbitmodels[i]->GetMetalnessTexture()->GetSampler();
		imagesampler4->ImageView = rabbitmodels[i]->GetMetalnessTexture()->GetView();

		descriptorinfo5.combinedImageSampler = imagesampler4;

		VulkanDescriptor* cis4Descr = new VulkanDescriptor(descriptorinfo5);

		VulkanDescriptorSet* descriptorSet = new VulkanDescriptorSet(&m_VulkanDevice, m_DescriptorPool.get(), descrSetLayout, { bufferDescr, cisDescr, cis2Descr, cis3Descr, cis4Descr }, "GeometryDescSet");

		rabbitmodels[i]->SetDescriptorSet(descriptorSet);
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
	//TODO: find better way to load cubemaps
	auto cubeMapData = ModelLoading::LoadCubemap("res/textures/skybox/skybox.jpg");

	ModelLoading::TextureData texData{};
	texData.bpp = cubeMapData->pData[0]->bpp;
	texData.width = cubeMapData->pData[0]->width;
	texData.height = cubeMapData->pData[0]->height;

	size_t imageSize = texData.height * texData.width * 4;

	texData.pData = (unsigned char*)malloc(imageSize * 6);
	for (int i = 0; i < 6; i++)
	{
		memcpy(texData.pData + i * imageSize, cubeMapData->pData[i]->pData, imageSize);
	}

	skyboxTexture = new VulkanTexture(&m_VulkanDevice, (TextureData*)&texData, TextureFlags::Color | TextureFlags::Read | TextureFlags::CubeMap | TextureFlags::TransferDst, Format::R8G8B8A8_UNORM_SRGB, "skybox");

	//TODO: pls no
	ModelLoading::FreeCubemap(cubeMapData);
	free(texData.pData);
}

void Renderer::loadModels()
{
	defaultBoxModel = ModelLoading::LoadScene("res/meshes/simpleShapes/box.obj");
	defaultSphereModel = ModelLoading::LoadScene("res/meshes/simpleShapes/sphere.obj");

	{
		auto testScene2 = ModelLoading::LoadScene("res/meshes/terrain/terrain.obj");
		RabbitModel* model = new RabbitModel(m_VulkanDevice, testScene2->pObjects[0]);
		auto mesh2 = model->GetMesh();
		mesh2.scale = { 30.f, 1.f, 30.f };
		mesh2.position = { 0.f, -2.f, 0.f };
		model->SetMesh(mesh2);
		rabbitmodels.push_back(model);
	}

	auto testScene2 = ModelLoading::LoadScene("res/meshes/cottage/Cottage_FREE.obj");
	{
		RabbitModel* model = new RabbitModel(m_VulkanDevice, testScene2->pObjects[0]);
		auto mesh2 = model->GetMesh();
		mesh2.position = { 5.f, -2.f, 0.f };
		model->SetMesh(mesh2);
		rabbitmodels.push_back(model);
	}

	//those first 4 represent lights :D
 	//AddSimpleMesh(SimpleMeshType::SphereMesh, { 7.0f, 1.0f, 7.0f }, 0.15f);
 	//AddSimpleMesh(SimpleMeshType::SphereMesh, { -7.0f, 1.0f, 7.0f }, 0.15f);
 	//AddSimpleMesh(SimpleMeshType::SphereMesh, { -10.0f, 2.0f, -10.0f }, 0.15f);
 	//AddSimpleMesh(SimpleMeshType::SphereMesh, { -14, 3, 13 }, 1.5f);
// 	//
// 	//
 	AddSimpleMesh(SimpleMeshType::SphereMesh, { -8, 1.8, 7.6 }, 2.5f);
// 	////
 	AddSimpleMesh(SimpleMeshType::BoxMesh, { -1, 1.8, 1 }, 0.5f);
// 	//
 	AddSimpleMesh(SimpleMeshType::BoxMesh, { -2.5, 1.8, 1 }, 0.5f);
// 	////
 	AddSimpleMesh(SimpleMeshType::BoxMesh, { -3, 1.8, -1.5 }, 0.5f);

	AddSimpleMesh(SimpleMeshType::BoxMesh, { 3, 10, 12 }, 1.3f);
	//ModelLoading::FreeScene(testScene2);
	//ModelLoading::FreeScene(testScene);
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
	std::vector<VkClearValue> clearValues(renderTargetCount);
	auto& renderTargets = m_StateManager->GetRenderTargets();

	for (size_t i = 0; i < renderTargetCount; i++)
	{
		auto format = renderTargets[i]->GetVkFormat();
		clearValues[i] = GetVkClearColorValueForFormat(format);
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

void Renderer::BindGraphicsPipeline(bool isCopyToSwapChain)
{

	if (!m_StateManager->GetPipelineDirty())
	{
		m_StateManager->GetPipeline()->Bind(GetCurrentCommandBuffer());
	}
	else
	{
		//TODO: take care of this really, optimize for most performance
		auto pipelineInfo = m_StateManager->GetPipelineInfo();

		auto& attachments = m_StateManager->GetRenderTargets();
		auto depthStencil = m_StateManager->GetDepthStencil();
		auto renderPassInfo = m_StateManager->GetRenderPassInfo();
		VulkanRenderPass* renderpass = PipelineManager::instance().FindOrCreateRenderPass(m_VulkanDevice, attachments, depthStencil, *renderPassInfo);
		m_StateManager->SetRenderPass(renderpass);

		VulkanFramebuffer* framebuffer;
		if (isCopyToSwapChain)
		{
			framebuffer = PipelineManager::instance().FindOrCreateFramebuffer(m_VulkanDevice, attachments, depthStencil, renderpass, GetUpscaledWidth, GetUpscaledHeight);
		}
		else
		{
			framebuffer = PipelineManager::instance().FindOrCreateFramebuffer(m_VulkanDevice, attachments, depthStencil, renderpass, GetNativeWidth, GetNativeHeight);
		}
		m_StateManager->SetFramebuffer(framebuffer);

		pipelineInfo->renderPass = m_StateManager->GetRenderPass();
		auto pipeline = PipelineManager::instance().FindOrCreateGraphicsPipeline(m_VulkanDevice, *pipelineInfo);
		m_StateManager->SetPipeline(pipeline);

		pipeline->Bind(GetCurrentCommandBuffer());
		m_StateManager->SetPipelineDirty(false);
	}
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

void Renderer::DrawGeometry(std::vector<RabbitModel*>& bucket)
{
	BindGraphicsPipeline();

	BeginRenderPass({ GetNativeWidth, GetNativeHeight });

	BindUBO();

	int currentModel = 0;

	for (auto model : bucket)
	{
		model->Bind(GetCurrentCommandBuffer());

		//bind geometry descriptors from models
		vkCmdBindDescriptorSets(
			GetCurrentCommandBuffer(),
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			*m_StateManager->GetPipeline()->GetPipelineLayout(),
			0,
			1,
			model->GetDescriptorSet()->GetVkDescriptorSet(),
			0,
			nullptr);

		BindModelMatrix(model);

		DrawIndicesIndirect(model->GetIndexCount(), currentModel);

		currentModel++;
	}

	geomDataIndirectDraw->FillBuffer(indexedDataBuffer, currentModel * sizeof(IndexIndirectDrawData));

	EndRenderPass();
}


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
		
		m_VertexUploadBuffer->FillBuffer(vertex.data(), /*this offset is from skybox, TODO: automate this*/ sizeof(Vertex) * 36, vertex.size() * sizeof(Vertex));

		BindVertexData(sizeof(Vertex) * 36);

		BindModelMatrix(model);

		vkCmdDraw(GetCurrentCommandBuffer(), 24, 1, 0, 0);
	}

	EndRenderPass();
}

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

void Renderer::FillTheLightParam(LightParams& lightParam, rabbitVec4f position, rabbitVec3f color, float radius)
{
	lightParam.position[0] = position.x;
	lightParam.position[1] = position.y;
	lightParam.position[2] = position.z;
	lightParam.position[3] = position.w;
	lightParam.color[0] = color.x;
	lightParam.color[1] = color.y;
	lightParam.color[2] = color.z;
	lightParam.radius = radius;
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

void Renderer::DrawFullScreenQuad()
{
	BindGraphicsPipeline();

	BindDescriptorSets();

	BeginRenderPass({ GetNativeWidth, GetNativeHeight });

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

void Renderer::ImageTransitionToPresent()
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = GetVkImageLayoutFrom(ResourceState::RenderTarget);
	barrier.newLayout = GetVkImageLayoutFrom(ResourceState::Present);
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_VulkanSwapchain->GetSwapChainImage(m_CurrentImageIndex)->GetImage();
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage = VK_ACCESS_TRANSFER_WRITE_BIT;
	VkPipelineStageFlags destinationStage = VK_ACCESS_TRANSFER_READ_BIT;

	vkCmdPipelineBarrier(
		GetCurrentCommandBuffer(),
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}

rabbitMat4f prevViewProjMatrix;
void Renderer::BindCameraMatrices(Camera* camera)
{	
	m_StateManager->UpdateUBOElement(UBOElement::PrevViewProjMatrix, 4, &prevViewProjMatrix);

	rabbitMat4f viewMatrix = camera->View();
	m_StateManager->UpdateUBOElement(UBOElement::ViewMatrix, 4, &viewMatrix);

	rabbitMat4f projMatrix = camera->Projection();
	m_StateManager->UpdateUBOElement(UBOElement::ProjectionMatrix, 4, &projMatrix);

	rabbitMat4f viewProjMatrix = projMatrix * viewMatrix;
	m_StateManager->UpdateUBOElement(UBOElement::ViewProjMatrix, 4, &viewProjMatrix);

	rabbitVec3f cameraPos = camera->GetPosition();
	m_StateManager->UpdateUBOElement(UBOElement::CameraPosition, 1, &cameraPos);

	rabbitMat4f viewProjInverse = glm::inverse(viewMatrix) * glm::inverse(projMatrix);
	m_StateManager->UpdateUBOElement(UBOElement::ViewProjInverse, 4, &viewProjInverse);

	prevViewProjMatrix = viewProjMatrix;
}

void Renderer::BindModelMatrix(RabbitModel* model)
{
	auto modelMesh = model->GetMesh();
	modelMesh.CalculateMatrix();	//Update model matrix
	auto modelMatrix = modelMesh.modelMatrix;

	SimplePushConstantData push{};
	push.modelMatrix = modelMatrix;
	push.id = model->GetId();

	push.useNormalMap = model->GetUseNormalMap();

	BindPushConstant(push);
}

void Renderer::BindUBO()
{	
	if (m_StateManager->GetUBODirty())
	{ 
		m_MainConstBuffer->FillBuffer(m_StateManager->GetUBO(), sizeof(UniformBufferObject));
		
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
	auto vertCode = ReadFile(GBUFFER_VERTEX_FILE_PATH);
	auto fragCode = ReadFile(GBUFFER_FRAGMENT_FILE_PATH);

	auto vertCode2 = ReadFile(PASSTHROUGH_VERTEX_FILE_PATH);
	auto fragCode2 = ReadFile(PASSTHROUGH_FRAGMENT_FILE_PATH);

	auto fragCode3 = ReadFile(PBR_FRAGMENT_FILE_PATH);
	auto fragCode4 = ReadFile(OUTLINE_ENTITY_FRAGMENT_FILE_PATH);

	auto vertCode3 = ReadFile(SKYBOX_VERTEX_FILE_PATH);
	auto fragCode5 = ReadFile(SKYBOX_FRAGMENT_FILE_PATH);

	auto fragCode6 = ReadFile(SSAO_FRAGMENT_FILE_PATH);
	auto fragCode7 = ReadFile(SSAOBLUR_FRAGMENT_FILE_PATH);

	auto fragCode8 = ReadFile(SIMPLE_GEOMETRY_FRAGMENT_PATH);
	auto vertCode4 = ReadFile(SIMPLE_GEOMETRY_VERTEX_PATH);

	auto compCode = ReadFile(RAY_TRACING_COMPUTE_FILE_PATH);
	auto compCode2 = ReadFile(FSREASU_COMPUTE_FILE_PATH);
	auto compCode3 = ReadFile(FSRRCAS_COMPUTE_FILE_PATH);

	auto compCode4 = ReadFile(TAA_COMPUTE_FILE_PATH);
	auto compCode5 = ReadFile(TAA_SHARPENER_COMPUTE_FILE_PATH);

	CreateShaderModule(vertCode, ShaderType::Vertex, "VS_GBuffer", nullptr);
	CreateShaderModule(fragCode, ShaderType::Fragment, "FS_GBuffer", nullptr);
	CreateShaderModule(vertCode2, ShaderType::Vertex, "VS_PassThrough", nullptr);
	CreateShaderModule(fragCode2, ShaderType::Fragment, "FS_PassThrough", nullptr);
	CreateShaderModule(fragCode3, ShaderType::Fragment, "FS_PBR", nullptr);
	CreateShaderModule(fragCode4, ShaderType::Fragment, "FS_OutlineEntity", nullptr);
	CreateShaderModule(vertCode3, ShaderType::Vertex, "VS_Skybox", nullptr);
	CreateShaderModule(fragCode5, ShaderType::Fragment, "FS_Skybox", nullptr);
	CreateShaderModule(fragCode6, ShaderType::Fragment, "FS_SSAO", nullptr);
	CreateShaderModule(fragCode7, ShaderType::Fragment, "FS_SSAOBlur", nullptr);
	CreateShaderModule(fragCode7, ShaderType::Fragment, "FS_SSAOBlur", nullptr);
	CreateShaderModule(fragCode8, ShaderType::Fragment, "FS_SimpleGeometry", nullptr);
	CreateShaderModule(vertCode4, ShaderType::Vertex, "VS_SimpleGeometry", nullptr);
	CreateShaderModule(compCode, ShaderType::Compute, "CS_RayTracingShadows", nullptr);
	CreateShaderModule(compCode2, ShaderType::Compute, "CS_FSR_EASU", nullptr);
	CreateShaderModule(compCode3, ShaderType::Compute, "CS_FSR_RCAS", nullptr);
	CreateShaderModule(compCode4, ShaderType::Compute, "CS_TAA", nullptr);
	CreateShaderModule(compCode5, ShaderType::Compute, "CS_TAASharpener", nullptr);
}

void Renderer::CreateShaderModule(const std::vector<char>& code, ShaderType type, const char* name, const char* codeEntry)
{
	ShaderInfo shaderInfo{};
	shaderInfo.CodeEntry = codeEntry;
	shaderInfo.Type = type;
	Shader* shader = new Shader(m_VulkanDevice, code.size(), code.data(), shaderInfo, name);
	m_Shaders[{name}] = shader;
}

void Renderer::createCommandBuffers() 
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

void Renderer::recreateSwapchain()
{
	VkExtent2D extent{};
	extent.width = GetUpscaledWidth;
	extent.height = GetUpscaledHeight;

	vkDeviceWaitIdle(m_VulkanDevice.GetGraphicDevice());
	m_VulkanSwapchain = std::make_unique<VulkanSwapchain>(m_VulkanDevice, extent);

	CreateUniformBuffers();
	CreateDescriptorPool();
}

void Renderer::ResourceBarrier(VulkanTexture* texture, ResourceState oldLayout, ResourceState newLayout)
{
	VkCommandBuffer commandBuffer = GetCurrentCommandBuffer();

	bool isDepth = oldLayout == ResourceState::DepthStencilRead || oldLayout == ResourceState::DepthStencilWrite; //TODO: investigate does this need newLayout to be checked

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
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == ResourceState::None && newLayout == ResourceState::TransferDst) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == ResourceState::RenderTarget && newLayout == ResourceState::TransferSrc) {
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == ResourceState::TransferSrc && newLayout == ResourceState::RenderTarget) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	else if (oldLayout == ResourceState::GenericRead && newLayout == ResourceState::TransferSrc) {
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == ResourceState::TransferSrc && newLayout == ResourceState::GenericRead) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == ResourceState::GenericRead && newLayout == ResourceState::TransferDst) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == ResourceState::None && newLayout == ResourceState::DepthStencilWrite)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == ResourceState::TransferDst && newLayout == ResourceState::GenericRead)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == ResourceState::TransferDst && newLayout == ResourceState::GeneralCompute)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}
	else if (oldLayout == ResourceState::None && newLayout == ResourceState::RenderTarget)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == ResourceState::RenderTarget && newLayout == ResourceState::GenericRead)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == ResourceState::GenericRead && newLayout == ResourceState::RenderTarget)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == ResourceState::DepthStencilWrite && newLayout == ResourceState::GenericRead)
	{
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == ResourceState::GenericRead && newLayout == ResourceState::DepthStencilWrite)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == ResourceState::GenericRead && newLayout == ResourceState::GeneralCompute)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}
	else if (oldLayout == ResourceState::RenderTarget && newLayout == ResourceState::GeneralCompute)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}
	else if (oldLayout == ResourceState::GeneralCompute && newLayout == ResourceState::RenderTarget)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		
		sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == ResourceState::GeneralCompute && newLayout == ResourceState::GenericRead)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		LOG_ERROR("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

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

	if (m_ImguiInitialized)
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Main debug frame");
		ImGui::Checkbox("Draw Bounding Box", &m_DrawBoundingBox);
		ImGui::Checkbox("Enable Entity picking", &m_RenderOutlinedEntity);
		ImGui::End();

		imguiReady = true;
	}

	GBufferPass gbuffer{};
	BoundingBoxPass bbPass{};
	SSAOPass ssao{};
	SSAOBlurPass ssaoBlur{};
	RTShadowsPass RTShadows{};
	LightingPass lighting{};
	SkyboxPass skybox{};
	OutlineEntityPass outlineEntity{};
	TAAPass taa{};
	TAASharpenerPass taaSharpener{};
	FSRPass fsr{};
	CopyToSwapchainPass copyToSwapchain{};

	UpdateDebugOptions();

	ExecuteRenderPass(gbuffer);
	ExecuteRenderPass(ssao);
	ExecuteRenderPass(ssaoBlur);
	ExecuteRenderPass(RTShadows);
	ExecuteRenderPass(skybox);
	ExecuteRenderPass(lighting);

	if (m_RenderOutlinedEntity)
	{
		ExecuteRenderPass(outlineEntity);
	}
	if (m_DrawBoundingBox)
	{ 
		ExecuteRenderPass(bbPass);
	}

	ExecuteRenderPass(taa);
	ExecuteRenderPass(taaSharpener);
	ExecuteRenderPass(fsr);

	ExecuteRenderPass(copyToSwapchain);

	EndCommandBuffer();
}


void Renderer::CreateUniformBuffers()
{
	m_MainConstBuffer = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::UniformBuffer, MemoryAccess::CPU, (uint64_t)sizeof(UniformBufferObject));
	m_LightParams = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::UniformBuffer, MemoryAccess::CPU, (uint64_t)sizeof(LightParams) * MAX_NUM_OF_LIGHTS);

	m_VertexUploadBuffer = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::VertexBuffer, MemoryAccess::CPU, MB_16); //64 mb fixed
}

void Renderer::CreateDescriptorPool()
{
	//TODO: see what to do with this, now its hard coded number of descriptors
	VulkanDescriptorPoolSize uboPoolSize{};
	uboPoolSize.Count = 30;
	uboPoolSize.Type = DescriptorType::UniformBuffer;

	VulkanDescriptorPoolSize samImgPoolSize{};
	samImgPoolSize.Count = 30;
	samImgPoolSize.Type = DescriptorType::SampledImage;

	VulkanDescriptorPoolSize sPoolSize{};
	sPoolSize.Count = 30;
	sPoolSize.Type = DescriptorType::Sampler;

	VulkanDescriptorPoolSize cisPoolSize{};
	cisPoolSize.Count = 30;
	cisPoolSize.Type = DescriptorType::CombinedSampler;

	VulkanDescriptorPoolSize siPoolSize{};
	siPoolSize.Count = 30;
	siPoolSize.Type = DescriptorType::StorageImage;

	VulkanDescriptorPoolSize sbPoolSize{};
	sbPoolSize.Count = 30;
	sbPoolSize.Type = DescriptorType::StorageBuffer;

	VulkanDescriptorPoolInfo vulkanDescriptorPoolInfo{};

	vulkanDescriptorPoolInfo.DescriptorSizes = { uboPoolSize, cisPoolSize, siPoolSize, sbPoolSize, samImgPoolSize, sPoolSize };

	vulkanDescriptorPoolInfo.MaxSets = 200;

	m_DescriptorPool = std::make_unique<VulkanDescriptorPool>(&m_VulkanDevice, vulkanDescriptorPoolInfo);
}

//SSAO
float lerp(float a, float b, float f)
{
	return a + f * (b - a);
}
void Renderer::InitSSAO()
{
	SSAOParamsBuffer = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::UniformBuffer, MemoryAccess::CPU, (uint64_t)sizeof(SSAOParams));

	SSAOTexture = new VulkanTexture(&m_VulkanDevice, GetNativeWidth, GetNativeHeight, TextureFlags::RenderTarget | TextureFlags::Read, Format::R32_SFLOAT, "SSAO");
	SSAOBluredTexture = new VulkanTexture(&m_VulkanDevice, GetNativeWidth, GetNativeHeight, TextureFlags::RenderTarget | TextureFlags::Read, Format::R32_SFLOAT, "SSAO");

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

	size_t bufferSize = 1024;
	SSAOSamplesBuffer = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::UniformBuffer, MemoryAccess::CPU, bufferSize);
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

	SSAONoiseTexture = new VulkanTexture(&m_VulkanDevice, &texData, TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst, Format::R32G32B32A32_FLOAT, "ssaoNoise");
		
	//init ssao params
	ssaoParams.radius = 0.5f;
	ssaoParams.bias = 0.025f;
	ssaoParams.resWidth = GetNativeWidth;
	ssaoParams.resHeight = GetNativeHeight;
	ssaoParams.kernelSize = 48;
	ssaoParams.ssaoOn = false;
}

void Renderer::InitLights()
{
	FillTheLightParam(lightParams[0], { 7.0f, 1.0f, 7.0f, 0.0f }, { 0.f, 0.f, 1.f }, 5.f);
	FillTheLightParam(lightParams[1], { -7.0f, 1.0f, 7.0f, 0.0f }, { 1.f, 0.f, 0.f }, 5.f);
	FillTheLightParam(lightParams[2], { -10.0f, 2.0f, -10.0f, 0.0f }, { 1.f, 0.6f, 0.2f }, 10.f);
	FillTheLightParam(lightParams[3], { -14.f, 3.f, 13.f, 0.0f }, { 1.f, 1.0f, 1.0f }, 30.f);
}

void Renderer::InitMeshDataForCompute()
{
	std::vector<Triangle> triangles;
	std::vector<rabbitVec4f> verticesFinal;

	uint32_t offset = 0;
	for (int i = 1; i < rabbitmodels.size(); i++)
	{
		auto& model = rabbitmodels[i];
		auto vertices = model->GetVertices();
		auto indices = model->GetIndices();

		auto modelMesh = model->GetMesh();
		modelMesh.CalculateMatrix();	//Update model matrix
		auto modelMatrix = modelMesh.modelMatrix;

		for (int k = 0; k < vertices.size(); k++)
		{
			rabbitVec4f position = modelMatrix * rabbitVec4f{ vertices[k].position, 1 };
			verticesFinal.push_back(position);
		}

		//will not work for multiple objects, need to calculate offset of vertex buffer all vertices + current index
		for (int j = 0; j < indices.size(); j+=3)
		{
			Triangle tri;
			tri.indices[0] = offset + indices[j];
			tri.indices[1] = offset + indices[j+1];
			tri.indices[2] = offset + indices[j+2];

			triangles.push_back(tri);
		}

		offset += vertices.size();
	}

	auto node = CreateBVH(verticesFinal, triangles);
	
	uint32_t* triIndices;
	uint32_t indicesNum = 0;

	CacheFriendlyBVHNode* root;
	uint32_t nodeNum = 0;

	CreateCFBVH(triangles.data(), node, &triIndices, &indicesNum, &root, &nodeNum);

	vertexBuffer = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::StorageBuffer, MemoryAccess::GPU, verticesFinal.size() * sizeof(rabbitVec4f));
	trianglesBuffer = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::StorageBuffer, MemoryAccess::GPU, triangles.size() * sizeof(Triangle));
	triangleIndxsBuffer = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::StorageBuffer, MemoryAccess::GPU, indicesNum * sizeof(uint32_t));
	cfbvhNodesBuffer = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::StorageBuffer, MemoryAccess::GPU, nodeNum * sizeof(CacheFriendlyBVHNode));
	
	vertexBuffer->FillBuffer(verticesFinal.data(), verticesFinal.size() * sizeof(rabbitVec4f));
	trianglesBuffer->FillBuffer(triangles.data(), triangles.size() * sizeof(Triangle));
	triangleIndxsBuffer->FillBuffer(triIndices, indicesNum * sizeof(uint32_t));
	cfbvhNodesBuffer->FillBuffer(root, nodeNum * sizeof(CacheFriendlyBVHNode));

	//meshDataSSBO->FillBuffer(meshdata.data(), meshdata.size() * sizeof(rabbitVec4f));
	//meshOffsetsSSBO->FillBuffer(offsets.data(), offsets.size() * sizeof(int));

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
	vkCmdDrawIndexedIndirect(GetCurrentCommandBuffer(), geomDataIndirectDraw->GetBuffer(), offset * sizeof(IndexIndirectDrawData), 1, sizeof(IndexIndirectDrawData));

	indexedDataBuffer[offset].firstIndex = 0;
	indexedDataBuffer[offset].firstInstance = 0;
	indexedDataBuffer[offset].indexCount = count;
	indexedDataBuffer[offset].instanceCount = 1;
	indexedDataBuffer[offset].vertexOffset = 0;
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
