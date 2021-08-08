#pragma once
#include "common.h"
#include "Logger/Logger.h"

#include <unordered_map>
#include <string>
#include <vulkan/vulkan.h>
#include <optional>

#include "Vulkan/Include/VulkanWrapper.h"
#include "Window.h"
#include "Model/ModelLoading.h"


class Camera;
class EntityManager;
struct GLFWwindow;
struct Vertex;
class VulkanDevice;
class Entity;
class Shader;

class Renderer
{
    SingletonClass(Renderer)

private:
	Camera*									MainCamera{};
	
	VulkanDevice							m_VulkanDevice{};
	std::unique_ptr<VulkanSwapchain>		m_VulkanSwapchain;
	std::unique_ptr<VulkanDescriptorPool>	m_DescriptorPool;
	std::vector<Shader*>					m_Shaders;
	std::vector<VkCommandBuffer>			m_CommandBuffers;
	
	std::vector<VulkanDescriptorSet*>		m_DescriptorSets;
	std::vector<VulkanBuffer*>				m_UniformBuffers;

	VulkanPipeline*							m_CurrentGraphicsPipeline;
	VulkanRenderPass*						m_CurrentRenderPass;

	int										m_CurrentImageIndex = 0;

	Entity*												testEntity;
	ModelLoading::SceneData*							testScene;
	std::vector<std::unique_ptr<RabbitModel>>			rabbitmodels;
	
	void loadModels();
	void LoadAndCreateShaders();
	void CreateShaderModule(const std::vector<char>& code, ShaderType type, const char* name, const char* codeEntry);
	void CreateRenderPasses();
	void CreateMainPhongLightingPipeline();
	void createCommandBuffers();
	void recreateSwapchain();
	void RecordCommandBuffer(int imageIndex);
	void UpdateUniformBuffer(uint32_t currentImage);
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();

	template <typename T>
	void BindPushConstant(int imageIndex, ShaderType shaderType, T& push)
	{
		vkCmdPushConstants(m_CommandBuffers[imageIndex], 
			*(m_CurrentGraphicsPipeline->GetPipelineLayout()), 
			GetVkShaderStageFrom(shaderType), 
			0, 
			sizeof(T), 
			&push);
	}

	void SetCurrentImageIndex(int imageIndex) { m_CurrentImageIndex = imageIndex; }
	void BeginRenderPass(VulkanRenderPass* renderPass);
	void EndRenderPass();

	void BindGraphicsPipeline(VulkanPipeline* pipeline);

	void BeginCommandBuffer();
	void EndCommandBuffer();

	//helper functions
	std::vector<char> ReadFile(const std::string& filepath);
public:
    bool m_FramebufferResized = false;

	bool Init();
	bool Shutdown();
	void Clear() const;
    void Draw(float dt);
    void DrawFrame();

};
