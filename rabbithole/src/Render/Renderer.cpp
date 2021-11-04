#define GLFW_INCLUDE_VULKAN

#include "Renderer.h"
#include "Render/Camera.h"
#include "Render/Window.h"
#include "Vulkan/Shader.h"
#include "ECS/EntityManager.h"
#include "Core/Application.h"
#include "Vulkan/Include/VulkanWrapper.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <vector>

#include <iostream>
#include <fstream>
#include <sstream>
#include <set>

#include "Model/ModelLoading.h"

float sranje = 0.f;

#define MAIN_VERTEX_FILE_PATH		"res/shaders/VS_PhongBasicTest.spv"
#define MAIN_FRAGMENT_FILE_PATH		"res/shaders/FS_PhongBasicTest.spv"



bool Renderer::Init()
{
	MainCamera = new Camera();
	MainCamera->Init();
	testEntity = new Entity();

	m_StateManager = new VulkanStateManager();

	testScene = ModelLoading::LoadScene("res/meshes/viking_room/viking_room.obj");
	size_t verticesCount = 0;
	std::vector<uint16_t> offsets{ 0 };
	for (size_t i = 0; i < testScene->numObjects; i++)
	{
		verticesCount += testScene->pObjects[i]->mesh->numVertices;
		offsets.push_back(testScene->pObjects[i]->mesh->numVertices * sizeof(Vertex));
	}

	loadModels();
	LoadAndCreateShaders();
	recreateSwapchain();
	createCommandBuffers();

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

	sranje += 0.25f * dt;
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

void Renderer::loadModels()
{
	// improvised to see how is engine rendering 10 models
	for (unsigned int i = 0; i < 10; i++)
	{
		RabbitModel* model2 = new RabbitModel(m_VulkanDevice, testScene->pObjects[0]);
		auto matrix = rabbitMat4f{ 1.f };
		matrix = glm::translate(matrix, rabbitVec3f(0.f, i*2.f, 0.f));
		model2->SetModelMatrix(matrix);
		rabbitmodels.push_back(model2);
	}
}

void Renderer::BeginRenderPass()
{
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_StateManager->GetRenderPass()->GetRenderPass(); //TODO: change names to GetVk.....
	renderPassInfo.framebuffer = m_StateManager->GetFramebuffer()->GetFramebuffer();

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_VulkanSwapchain->GetSwapChainExtent(); //only fullscreen draw for now

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(m_CommandBuffers[m_CurrentImageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void Renderer::EndRenderPass()
{
	vkCmdEndRenderPass(m_CommandBuffers[m_CurrentImageIndex]);
}

void Renderer::BindGraphicsPipeline()
{
    if (!m_StateManager->GetRenderPassDirty())
    {

    }
    else
    {
        auto attachments = m_StateManager->GetRenderTargets();
        auto depthStencil = m_StateManager->GetDepthStencil();
        auto renderPassInfo = m_StateManager->GetRenderPassInfo();
        VulkanRenderPass* renderpass = PipelineManager::instance().FindOrCreateRenderPass(m_VulkanDevice, attachments, depthStencil, *renderPassInfo);
        m_StateManager->SetRenderPass(renderpass);

        m_StateManager->SetRenderPassDirty(false);
    }

	if (!m_StateManager->GetPipelineDirty())
	{
		m_StateManager->GetPipeline()->Bind(m_CommandBuffers[m_CurrentImageIndex]);
	}
	else
	{
		//FIND OR CREATE PIPELINE
		auto pipelineInfo = m_StateManager->GetPipelineInfo();
		pipelineInfo->renderPass = m_StateManager->GetRenderPass();
		auto pipeline = PipelineManager::instance().FindOrCreateGraphicsPipeline(m_VulkanDevice, *pipelineInfo);
		m_StateManager->SetPipeline(pipeline);
		pipeline->Bind(m_CommandBuffers[m_CurrentImageIndex]);
		m_StateManager->SetPipelineDirty(false);
	}
}

void Renderer::DrawBucket(std::vector<RabbitModel*> bucket)
{
	BindGraphicsPipeline();

	BeginRenderPass();

	BindDescriptorSets();

	BindUBO();
	
	for (auto model : bucket)
	{
		model->Bind(m_CommandBuffers[m_CurrentImageIndex]);
		
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
	auto modelMatrix = model->GetModelMatrix();
	SimplePushConstantData push{};
	push.modelMatrix = modelMatrix;
	BindPushConstant(ShaderType::Vertex, push);
}

void Renderer::BindUBO()
{	
	if (m_StateManager->GetUBODirty())
	{ 
		void* data = m_UniformBuffers[m_CurrentImageIndex]->Map();
		memcpy(data, m_StateManager->GetUBO(), sizeof(UniformBufferObject));
		m_UniformBuffers[m_CurrentImageIndex]->Unmap();
		
		m_StateManager->SetUBODirty(false);
	}
}

void Renderer::BindDescriptorSets()
{
	//TODO: decide where to store descriptor sets, models or state manager?
	vkCmdBindDescriptorSets(m_CommandBuffers[m_CurrentImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, *m_StateManager->GetPipeline()->GetPipelineLayout(), 0, 1, m_DescriptorSets[m_CurrentImageIndex]->GetDescriptorSet(), 0, nullptr);
}

void Renderer::LoadAndCreateShaders()
{
	auto vertCode = ReadFile(MAIN_VERTEX_FILE_PATH);
	auto fragCode = ReadFile(MAIN_FRAGMENT_FILE_PATH);

	CreateShaderModule(vertCode, ShaderType::Vertex, "mainvertex", nullptr);
	CreateShaderModule(fragCode, ShaderType::Fragment, "mainfragment", nullptr);
}

void Renderer::CreateShaderModule(const std::vector<char>& code, ShaderType type, const char* name, const char* codeEntry)
{
	ShaderInfo shaderInfo{};
	shaderInfo.CodeEntry = codeEntry;
	shaderInfo.Type = type;
	Shader* shader = new Shader(m_VulkanDevice, code.size(), code.data(), shaderInfo, name);
	m_Shaders.push_back(shader);
}

void Renderer::CreateRenderPasses()
{
}

void Renderer::CreateMainPhongLightingPipeline() 
{
	PipelineConfigInfo pipelineConfig{};
	//BAD BAD BAD WTF is this, take care of this please
	pipelineConfig.vertexShader = m_Shaders[0];
	pipelineConfig.pixelShader = m_Shaders[1];
	pipelineConfig.renderPass = m_VulkanSwapchain->GetRenderPass();

	VulkanPipeline* pipeline = new VulkanPipeline(m_VulkanDevice, pipelineConfig);
	m_VulkanDevice.AddPipelineToCollection(pipeline, "phonglighting");
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

	CreateRenderPasses();
	CreateMainPhongLightingPipeline();
	
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
}

void Renderer::RecordCommandBuffer(int imageIndex)
{
	SetCurrentImageIndex(imageIndex);
	BeginCommandBuffer();

	m_StateManager->SetRenderPass(m_VulkanSwapchain->GetRenderPass());
	m_StateManager->SetFramebuffer(m_VulkanSwapchain->GetFrameBuffer(m_CurrentImageIndex));

	m_StateManager->SetCullMode(CullMode::None);
	m_StateManager->SetVertexShader(m_Shaders[0]);
	m_StateManager->SetPixelShader(m_Shaders[1]);

	BindCameraMatrices(MainCamera);
	
	DrawBucket(rabbitmodels);

	EndCommandBuffer();
}

void Renderer::UpdateUniformBuffer(uint32_t currentImage)
{

}


void Renderer::CreateUniformBuffers()
{
	m_UniformBuffers.resize(m_VulkanSwapchain->GetImageCount());
	for (size_t i = 0; i < m_VulkanSwapchain->GetImageCount(); i++)
	{
		m_UniformBuffers[i] = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::UniformBuffer, MemoryAccess::Host, (uint64_t)sizeof(UniformBufferObject));
	}
}

void Renderer::CreateDescriptorPool()
{
	VulkanDescriptorPoolSize uboPoolSize{};
	uboPoolSize.Count = m_VulkanSwapchain->GetImageCount();
	uboPoolSize.Type = DescriptorType::UniformBuffer;

	VulkanDescriptorPoolSize cisPoolSize{};
	cisPoolSize.Count = m_VulkanSwapchain->GetImageCount();
	cisPoolSize.Type = DescriptorType::CombinedSampler;

	VulkanDescriptorPoolInfo vulkanDescriptorPoolInfo{};
	vulkanDescriptorPoolInfo.DescriptorSizes = { uboPoolSize, cisPoolSize };
	vulkanDescriptorPoolInfo.MaxSets = static_cast<uint32_t>(m_VulkanSwapchain->GetImageCount());

	m_DescriptorPool = std::make_unique<VulkanDescriptorPool>(&m_VulkanDevice, vulkanDescriptorPoolInfo);
}

void Renderer::CreateDescriptorSets()
{
	size_t backBuffersCount = m_VulkanSwapchain->GetImageCount();
	m_DescriptorSets.resize(backBuffersCount);

	VulkanPipeline* pipeline = m_VulkanDevice.GetPipelineFromCollection({ "phonglighting" });

	for (size_t i = 0; i < backBuffersCount; i++)
	{
		std::vector<VulkanDescriptor*> descriptors;

		VulkanDescriptorInfo uniformBufferDescriptorInfo{};
		uniformBufferDescriptorInfo.Type = DescriptorType::UniformBuffer;
		uniformBufferDescriptorInfo.buffer = m_UniformBuffers[i];
		uniformBufferDescriptorInfo.Binding = 0;
		VulkanDescriptor uniformBufferDescriptor(uniformBufferDescriptorInfo);

		descriptors.push_back(&uniformBufferDescriptor);

		CombinedImageSampler combinedImageSampler;
		combinedImageSampler.ImageSampler = rabbitmodels[0]->GetTexture()->GetSampler();
		combinedImageSampler.ImageView = rabbitmodels[0]->GetTexture()->GetView();

		VulkanDescriptorInfo combinedImageSamplerDescriptorInfo{};
		combinedImageSamplerDescriptorInfo.Type = DescriptorType::CombinedSampler;
		combinedImageSamplerDescriptorInfo.combinedImageSampler = &combinedImageSampler;
		combinedImageSamplerDescriptorInfo.Binding = 1;

		VulkanDescriptor combinedImageSamplerDescriptor(combinedImageSamplerDescriptorInfo);

		descriptors.push_back(&combinedImageSamplerDescriptor);

 		m_DescriptorSets[i] = new VulkanDescriptorSet(&m_VulkanDevice, m_DescriptorPool.get(),
			pipeline->GetDescriptorSetLayout(), descriptors, "Main");
	}
}
