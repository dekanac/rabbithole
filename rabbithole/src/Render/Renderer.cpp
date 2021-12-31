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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <vector>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <set>

#include "Model/ModelLoading.h"

rabbitVec3f renderDebugOption;

#define GBUFFER_VERTEX_FILE_PATH			"res/shaders/VS_GBuffer.spv"
#define GBUFFER_FRAGMENT_FILE_PATH			"res/shaders/FS_GBuffer.spv"
#define PASSTHROUGH_VERTEX_FILE_PATH		"res/shaders/VS_PassThrough.spv"
#define PASSTHROUGH_FRAGMENT_FILE_PATH		"res/shaders/FS_PassThrough.spv"
#define PHONG_LIGHT_FRAGMENT_FILE_PATH		"res/shaders/FS_PhongBasicTest.spv"
#define OUTLINE_ENTITY_FRAGMENT_FILE_PATH	"res/shaders/FS_OutlineEntity.spv"

void Renderer::ExecuteRenderPass(RenderPass& renderpass)
{
	m_VulkanDevice.BeginLabel(m_CommandBuffers[m_CurrentImageIndex], renderpass.GetName());

	renderpass.DeclareResources(this);
	renderpass.Setup(this);
	renderpass.Render(this);

	m_VulkanDevice.EndLabel(m_CommandBuffers[m_CurrentImageIndex]);
}

bool Renderer::Init()
{
	MainCamera = new Camera();
	MainCamera->Init();
	testEntity = new Entity();
	testEntity->AddComponent<InputComponent>();

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

	albedoGBuffer = new VulkanTexture(&m_VulkanDevice, DEFAULT_WIDTH, DEFAULT_HEIGHT, TextureFlags::RenderTarget, Format::B8G8R8A8_UNORM, "albedo");
	normalGBuffer = new VulkanTexture(&m_VulkanDevice, DEFAULT_WIDTH, DEFAULT_HEIGHT, TextureFlags::RenderTarget, Format::R16G16B16A16_FLOAT, "normal");
	worldPositionGBuffer = new VulkanTexture(&m_VulkanDevice, DEFAULT_WIDTH, DEFAULT_HEIGHT, TextureFlags::RenderTarget, Format::R16G16B16A16_FLOAT, "wordlPosition");
	
	entityHelper = new VulkanTexture(&m_VulkanDevice, DEFAULT_WIDTH, DEFAULT_HEIGHT, TextureFlags::RenderTarget | TextureFlags::LinearTiling | TextureFlags::TransferSrc, Format::R32_UINT, "entityHelper");
	entityHelperBuffer = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::StorageBuffer, MemoryAccess::Host, DEFAULT_HEIGHT * DEFAULT_WIDTH * 4);

	lightingMain = new VulkanTexture(&m_VulkanDevice, DEFAULT_WIDTH, DEFAULT_HEIGHT, TextureFlags::RenderTarget, Format::R16G16B16A16_FLOAT, "lightingmain");

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
	descriptorinfo.buffer = m_UniformBuffer;

	VulkanDescriptor* bufferDescr = new VulkanDescriptor(descriptorinfo);

	for (size_t i = 0; i < rabbitmodels.size(); i++)
	{

		VulkanDescriptorInfo descriptorinfo2{};
		descriptorinfo2.Type = DescriptorType::CombinedSampler;
		descriptorinfo2.Binding = 1;

		CombinedImageSampler* imagesampler = new CombinedImageSampler();
		imagesampler->ImageSampler = rabbitmodels[i]->GetTexture()->GetSampler();
		imagesampler->ImageView = rabbitmodels[i]->GetTexture()->GetView();

		descriptorinfo2.combinedImageSampler = imagesampler;

		VulkanDescriptor* cisDescr = new VulkanDescriptor(descriptorinfo2);

		VulkanDescriptorInfo descriptorinfo3{};
		descriptorinfo3.Type = DescriptorType::CombinedSampler;
		descriptorinfo3.Binding = 2;

		CombinedImageSampler* imagesampler2 = new CombinedImageSampler();
		imagesampler2->ImageSampler = rabbitmodels[i]->GetNormalTexture()->GetSampler();
		imagesampler2->ImageView = rabbitmodels[i]->GetNormalTexture()->GetView();

		descriptorinfo3.combinedImageSampler = imagesampler2;

		VulkanDescriptor* cis2Descr = new VulkanDescriptor(descriptorinfo3);

		VulkanDescriptorSet* descriptorSet = new VulkanDescriptorSet(&m_VulkanDevice, m_DescriptorPool.get(), descrSetLayout, { bufferDescr, cisDescr, cis2Descr }, "GeometryDescSet");

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

void Renderer::loadModels()
{
	auto testScene = ModelLoading::LoadScene("res/meshes/cottage/Cottage_FREE.obj");
	auto testScene2 = ModelLoading::LoadScene("res/meshes/terrain/terrain.obj");	

	// improvised to see how is engine rendering 10 models
	for (unsigned int i = 0; i < testScene->numObjects; i++)
	{
		RabbitModel* model = new RabbitModel(m_VulkanDevice, testScene->pObjects[i]);
		Mesh mesh{};
		model->SetMesh(mesh);
		rabbitmodels.push_back(model);
	}

	{
		RabbitModel* model = new RabbitModel(m_VulkanDevice, testScene2->pObjects[0]);
		auto matrix = rabbitMat4f{ 1.f };
		Mesh mesh{};
		mesh.position = { 7.f, 0.1f, 9.f };
		model->SetMesh(mesh);
		rabbitmodels.push_back(model);
	}


	ModelLoading::FreeScene(testScene);
	ModelLoading::FreeScene(testScene2);
}

void Renderer::BeginRenderPass()
{
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_StateManager->GetRenderPass()->GetVkRenderPass();
	renderPassInfo.framebuffer = m_StateManager->GetFramebuffer()->GetVkFramebuffer();

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_VulkanSwapchain->GetSwapChainExtent(); //only fullscreen draw for now

	auto renderTargetCount = m_StateManager->GetRenderTargetCount();
	std::vector<VkClearValue> clearValues(renderTargetCount);

	for (size_t i = 0; i < renderTargetCount; i++)
	{
		clearValues[i].color = { 0.1f, 0.1f, 0.1f, 1.0f };
	}

	if (m_StateManager->HasDepthStencil())
	{
		renderTargetCount++;
		VkClearValue depthClearValue{};
		depthClearValue.depthStencil = { 1.0f, 0 };
		clearValues.push_back(depthClearValue);
	}

	renderPassInfo.clearValueCount = static_cast<uint32_t>(renderTargetCount);
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(m_CommandBuffers[m_CurrentImageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void Renderer::EndRenderPass()
{
	vkCmdEndRenderPass(m_CommandBuffers[m_CurrentImageIndex]);
	m_StateManager->Reset();
}

void Renderer::BindGraphicsPipeline()
{

	if (!m_StateManager->GetPipelineDirty())
	{
		m_StateManager->GetPipeline()->Bind(m_CommandBuffers[m_CurrentImageIndex]);
	}
	else
	{
		//TODO: take care of this really, optimize for most performance
		auto attachments = m_StateManager->GetRenderTargets();
		auto depthStencil = m_StateManager->GetDepthStencil();
		auto renderPassInfo = m_StateManager->GetRenderPassInfo();
		VulkanRenderPass* renderpass = PipelineManager::instance().FindOrCreateRenderPass(m_VulkanDevice, attachments, depthStencil, *renderPassInfo);
		m_StateManager->SetRenderPass(renderpass);

		VulkanFramebuffer* framebuffer = PipelineManager::instance().FindOrCreateFramebuffer(m_VulkanDevice, attachments, depthStencil, renderpass, Window::instance().GetExtent().width, Window::instance().GetExtent().height);
		m_StateManager->SetFramebuffer(framebuffer);

		auto pipelineInfo = m_StateManager->GetPipelineInfo();
		pipelineInfo->renderPass = m_StateManager->GetRenderPass();
		auto pipeline = PipelineManager::instance().FindOrCreateGraphicsPipeline(m_VulkanDevice, *pipelineInfo);
		m_StateManager->SetPipeline(pipeline);

		pipeline->Bind(m_CommandBuffers[m_CurrentImageIndex]);
		m_StateManager->SetPipelineDirty(false);
	}
}

void Renderer::DrawGeometry(std::vector<RabbitModel*> bucket)
{

	BindGraphicsPipeline();

	BeginRenderPass();

	BindUBO();
	
	for (auto model : bucket)
	{
		
		model->Bind(m_CommandBuffers[m_CurrentImageIndex]);

		//bind geometry descriptors from models
		vkCmdBindDescriptorSets(
			m_CommandBuffers[m_CurrentImageIndex],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			*m_StateManager->GetPipeline()->GetPipelineLayout(),
			0,
			1,
			model->GetDescriptorSet()->GetVkDescriptorSet(),
			0,
			nullptr);

		BindModelMatrix(model);

		model->Draw(m_CommandBuffers[m_CurrentImageIndex]);

	}

	EndRenderPass();
}

void Renderer::BeginCommandBuffer()
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VULKAN_API_CALL(vkBeginCommandBuffer(m_CommandBuffers[m_CurrentImageIndex], &beginInfo), "failed to begin recording command buffer!");
}

void Renderer::EndCommandBuffer()
{
	VULKAN_API_CALL(vkEndCommandBuffer(m_CommandBuffers[m_CurrentImageIndex]), "failed to end recording command buffer!");
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

	BeginRenderPass();

	vkCmdDraw(m_CommandBuffers[m_CurrentImageIndex], 3, 1, 0, 0);

	EndRenderPass();
}

void Renderer::UpdateEntityPickId()
{
	auto inputComponent = testEntity->GetComponent<InputComponent>();
	auto x = inputComponent->mouse_current_x;
	auto y = inputComponent->mouse_current_y;

	CopyImageToBuffer(entityHelper, entityHelperBuffer);
	void* data = entityHelperBuffer->Map();
	uint32_t* dataInt = (uint32_t*)data;

	int current_pixel = (y)*DEFAULT_WIDTH + x;
	if (current_pixel > 0 && current_pixel < DEFAULT_WIDTH * DEFAULT_HEIGHT)
	{
		PushEntityId push{};
		push.id = dataInt[current_pixel];
		BindPushConstant(push);
	}

	entityHelperBuffer->Unmap();

}

void Renderer::CopyToSwapChain()
{

	BindGraphicsPipeline();

#ifdef RABBITHOLE_USING_IMGUI
	if (!m_ImguiInitialized)
	{
		InitImgui();
		m_ImguiInitialized = true;
	}
#endif

	BindDescriptorSets();

	BeginRenderPass();

	//COPY TO SWAPCHAIN
	vkCmdDraw(m_CommandBuffers[m_CurrentImageIndex], 3, 1, 0, 0);

#ifdef RABBITHOLE_USING_IMGUI
	//DRAW UI(imgui)
	if (m_ImguiInitialized)
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Test");
		ImGui::End();

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_CommandBuffers[m_CurrentImageIndex]);
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
		m_CommandBuffers[m_CurrentImageIndex],
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}

void Renderer::BindCameraMatrices(Camera* camera)
{
	auto viewMatrix = camera->View();
	m_StateManager->UpdateUBOElement(UBOElement::ViewMatrix, 4, &viewMatrix);

	auto projMatrix = camera->Projection();
	m_StateManager->UpdateUBOElement(UBOElement::ProjectionMatrix, 4, &projMatrix);

	auto cameraPos = camera->GetPosition();
	m_StateManager->UpdateUBOElement(UBOElement::CameraPosition, 1, &cameraPos);
}

void Renderer::BindModelMatrix(RabbitModel* model)
{
	auto modelMesh = model->GetMesh();
	modelMesh.CalculateMatrix();	//Update model matrix
	auto modelMatrix = modelMesh.modelMatrix;

	SimplePushConstantData push{};
	push.modelMatrix = modelMatrix;
	push.id = model->GetId();
	BindPushConstant(push);
}

void Renderer::BindUBO()
{	
	if (m_StateManager->GetUBODirty())
	{ 
		void* data = m_UniformBuffer->Map();
		memcpy(data, m_StateManager->GetUBO(), sizeof(UniformBufferObject));
		m_UniformBuffer->Unmap();
		
		m_StateManager->SetUBODirty(false);
	}
}

void Renderer::BindDescriptorSets()
{
	auto descriptorSet = m_StateManager->FinalizeDescriptorSet(m_VulkanDevice, m_DescriptorPool.get());
	//TODO: decide where to store descriptor sets, models or state manager?
	vkCmdBindDescriptorSets(m_CommandBuffers[m_CurrentImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, *m_StateManager->GetPipeline()->GetPipelineLayout(), 0, 1, descriptorSet->GetVkDescriptorSet(), 0, nullptr);
}

void Renderer::LoadAndCreateShaders()
{
	auto vertCode = ReadFile(GBUFFER_VERTEX_FILE_PATH);
	auto fragCode = ReadFile(GBUFFER_FRAGMENT_FILE_PATH);

	auto vertCode2 = ReadFile(PASSTHROUGH_VERTEX_FILE_PATH);
	auto fragCode2 = ReadFile(PASSTHROUGH_FRAGMENT_FILE_PATH);

	auto fragCode3 = ReadFile(PHONG_LIGHT_FRAGMENT_FILE_PATH);
	auto fragCode4 = ReadFile(OUTLINE_ENTITY_FRAGMENT_FILE_PATH);

	CreateShaderModule(vertCode, ShaderType::Vertex, "VS_GBuffer", nullptr);
	CreateShaderModule(fragCode, ShaderType::Fragment, "FS_GBuffer", nullptr);
	CreateShaderModule(vertCode2, ShaderType::Vertex, "VS_PassThrough", nullptr);
	CreateShaderModule(fragCode2, ShaderType::Fragment, "FS_PassThrough", nullptr);
	CreateShaderModule(fragCode3, ShaderType::Fragment, "FS_PhongBasicTest", nullptr);
	CreateShaderModule(fragCode4, ShaderType::Fragment, "FS_OutlineEntity", nullptr);
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
	auto extent = Window::instance().GetExtent();
	while (extent.width == 0 || extent.height == 0)
	{
		extent = Window::instance().GetExtent();
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_VulkanDevice.GetGraphicDevice());
	m_VulkanSwapchain = std::make_unique<VulkanSwapchain>(m_VulkanDevice, extent);

	CreateUniformBuffers();
	CreateDescriptorPool();
}

void Renderer::ResourceBarrier(VulkanTexture* texture, ResourceState oldLayout, ResourceState newLayout)
{
	VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentImageIndex];

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = GetVkImageLayoutFrom(oldLayout);
	barrier.newLayout = GetVkImageLayoutFrom(newLayout);
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = texture->GetResource()->GetImage();
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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

	GBufferPass gbuffer{};
	LightingPass lighting{};
	OutlineEntityPass outlineEntityPass{};
	CopyToSwapchainPass copytoswapchain{};

	UpdateDebugOptions();
	ExecuteRenderPass(gbuffer);
	ExecuteRenderPass(lighting);
	if (m_RenderOutlinedEntity)
	{
		ExecuteRenderPass(outlineEntityPass);
	}
	ExecuteRenderPass(copytoswapchain);

	EndCommandBuffer();
}


void Renderer::CreateUniformBuffers()
{
	m_UniformBuffer = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::UniformBuffer, MemoryAccess::Host, (uint64_t)sizeof(UniformBufferObject) + 32/*just to be sure :D*/);
	m_LightParams = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::UniformBuffer, MemoryAccess::Host, (uint64_t)sizeof(LightParams) * MAX_NUM_OF_LIGHTS);
}

void Renderer::CreateDescriptorPool()
{
	//TODO: see what to do with this, now its hard coded number of descriptors
	VulkanDescriptorPoolSize uboPoolSize{};
	uboPoolSize.Count = 10;
	uboPoolSize.Type = DescriptorType::UniformBuffer;

	VulkanDescriptorPoolSize cisPoolSize{};
	cisPoolSize.Count = 10;
	cisPoolSize.Type = DescriptorType::CombinedSampler;

	VulkanDescriptorPoolInfo vulkanDescriptorPoolInfo{};
	vulkanDescriptorPoolInfo.DescriptorSizes = { uboPoolSize, cisPoolSize };
	vulkanDescriptorPoolInfo.MaxSets = 10; //static_cast<uint32_t>(m_VulkanSwapchain->GetImageCount() * 2);

	m_DescriptorPool = std::make_unique<VulkanDescriptorPool>(&m_VulkanDevice, vulkanDescriptorPoolInfo);
}

void Renderer::BindViewport(float x, float y, float width, float height)
{
#ifdef DYNAMIC_SCISSOR_AND_VIEWPORT_STATES
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

	vkCmdSetViewport(m_CommandBuffers[m_CurrentImageIndex], 0, 1, &viewport);
	//for now scissor is not used separately, i'll just bind both here
	vkCmdSetScissor(m_CommandBuffers[m_CurrentImageIndex], 0, 1, &scissor);
#else
	m_StateManager->SetViewport(x, y, width, height);
#endif
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