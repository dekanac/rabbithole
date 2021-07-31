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

#define MAIN_VERTEX_FILE_PATH		"res/shaders/vert.spv"
#define MAIN_FRAGMENT_FILE_PATH		"res/shaders/frag.spv"

struct UniformBufferObject
{
	alignas(16) rabbitMat4f view;
	alignas(16) rabbitMat4f proj;
};

bool Renderer::Init()
{
	MainCamera = new Camera();
	MainCamera->Init();
	testEntity = new Entity();

	testScene = ModelLoading::LoadScene("res/meshes/box/box.obj");
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
    return true;
}

void Renderer::Clear() const
{
    
}

void Renderer::Draw(float dt)
{
	MainCamera->Update(dt);

	sranje += 0.5 * dt;
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

	recordCommandBuffer(imageIndex);
	UpdateUniformBuffer(imageIndex);


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
	for (unsigned int i = 0; i < testScene->numObjects; i++)
	{
		rabbitmodels.push_back(std::make_unique<RabbitModel>(m_VulkanDevice, testScene->pObjects[i]));
	}
}

void Renderer::BeginRenderPass(VkRenderPass& renderPass)
{
	m_CurrentRenderPass = &renderPass;
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = *m_CurrentRenderPass;
	renderPassInfo.framebuffer = m_VulkanSwapchain->GetFrameBuffer(m_CurrentImageIndex);

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_VulkanSwapchain->GetSwapChainExtent();

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

void Renderer::BindGraphicsPipeline(VulkanPipeline* pipeline)
{
	m_CurrentGraphicsPipeline = pipeline;
	pipeline->Bind(m_CommandBuffers[m_CurrentImageIndex]);
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

void Renderer::CreateMainPhongLightingPipeline() 
{
	PipelineConfigInfo pipelineConfig{};
	VulkanPipeline::DefaultPipelineConfigInfo(
		pipelineConfig,
		m_VulkanSwapchain->GetWidth(),
		m_VulkanSwapchain->GetHeight());

	m_DescriptorSetLayout = new VulkanDescriptorSetLayout(&m_VulkanDevice, m_Shaders, "Main");

	pipelineConfig.renderPass = m_VulkanSwapchain->GetRenderPass();
	pipelineConfig.pipelineLayout = *m_DescriptorSetLayout->GetPipelineLayout();

	m_VulkanPipeline = std::make_unique<VulkanPipeline>(m_VulkanDevice, m_Shaders, pipelineConfig);
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

	CreateMainPhongLightingPipeline();
	
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
}

void Renderer::recordCommandBuffer(int imageIndex)
{
	SetCurrentImageIndex(imageIndex);
	BeginCommandBuffer();

	VkRenderPass renderPass = m_VulkanSwapchain->GetRenderPass();
	BeginRenderPass(renderPass);
	BindGraphicsPipeline(m_VulkanPipeline.get());

	SimplePushConstantData push{};
	push.cameraPosition = MainCamera->GetPosition();

	for(unsigned int i = 0; i < testScene->numObjects; i++) 
	{ 
		rabbitmodels[i]->Bind(m_CommandBuffers[imageIndex]);

		vkCmdBindDescriptorSets(m_CommandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, *m_VulkanPipeline->GetPipelineLayout(), 0, 1, m_DescriptorSets[imageIndex]->GetDescriptorSet(), 0, nullptr);

		for (int j = 1; j < 400; j++)
		{
			push.model = glm::translate(rabbitMat4f{ 1.f } , rabbitVec3f(j * 3.f, 0.f, 0.f));
			push.model = glm::rotate(push.model, 3.14f, rabbitVec3f(1.0f, 0.0f, 0.0f));
			push.model = glm::rotate(push.model, sranje, rabbitVec3f(0.0f, 1.0f, 0.0f));
			vkCmdPushConstants(m_CommandBuffers[imageIndex], *(m_VulkanPipeline->GetPipelineLayout()), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SimplePushConstantData), &push);
			rabbitmodels[i]->Draw(m_CommandBuffers[imageIndex]);
		}
	}

	EndRenderPass();
	EndCommandBuffer();
}

void Renderer::UpdateUniformBuffer(uint32_t currentImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.view = MainCamera->View();
	ubo.proj = MainCamera->Projection();

	void* data = m_UniformBuffers[currentImage]->Map();
	memcpy(data, &ubo, sizeof(ubo));
	m_UniformBuffers[currentImage]->Unmap();
}


void Renderer::CreateUniformBuffers()
{
	m_UniformBuffers.resize(m_VulkanSwapchain->GetImageCount());
	for (size_t i = 0; i < m_VulkanSwapchain->GetImageCount(); i++)
	{
		VulkanBufferInfo bufferInfo{};
		bufferInfo.usageFlags = BufferUsageFlags::UniformBuffer;
		bufferInfo.memoryAccess = MemoryAccess::Host;
		bufferInfo.size = (uint32_t)sizeof(UniformBufferObject);
		m_UniformBuffers[i] = new VulkanBuffer(&m_VulkanDevice, bufferInfo);
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
		combinedImageSampler.ImageSampler = rabbitmodels[0]->imageSampler;
		combinedImageSampler.ImageView = rabbitmodels[0]->imageView;

		VulkanDescriptorInfo combinedImageSamplerDescriptorInfo{};
		combinedImageSamplerDescriptorInfo.Type = DescriptorType::CombinedSampler;
		combinedImageSamplerDescriptorInfo.combinedImageSampler = &combinedImageSampler;
		combinedImageSamplerDescriptorInfo.Binding = 1;

		VulkanDescriptor combinedImageSamplerDescriptor(combinedImageSamplerDescriptorInfo);

		descriptors.push_back(&combinedImageSamplerDescriptor);

 		m_DescriptorSets[i] = new VulkanDescriptorSet(&m_VulkanDevice, m_DescriptorPool.get(),
 			m_DescriptorSetLayout, descriptors, "Main");
	}
}
