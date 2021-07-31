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
	std::unique_ptr<VulkanPipeline>			m_VulkanPipeline;
	std::vector<Shader*>					m_Shaders;
	std::vector<VkCommandBuffer>			m_CommandBuffers;
	
	VulkanDescriptorSetLayout*				m_DescriptorSetLayout;
	std::vector<VulkanDescriptorSet*>		m_DescriptorSets;
	std::vector<VulkanBuffer*>				m_UniformBuffers;

	std::vector<std::unique_ptr<RabbitModel>>			rabbitmodels;
	Entity*									testEntity;
	ModelLoading::SceneData*				testScene;
	VulkanPipeline* m_CurrentGraphicsPipeline;
	VkRenderPass* m_CurrentRenderPass;

	int m_CurrentImageIndex = 0;

	void loadModels();
	void LoadAndCreateShaders();
	void CreateShaderModule(const std::vector<char>& code, ShaderType type, const char* name, const char* codeEntry);
	void CreateMainPhongLightingPipeline();
	void createCommandBuffers();
	void recreateSwapchain();
	void RecordCommandBuffer(int imageIndex);
	void UpdateUniformBuffer(uint32_t currentImage);
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();

	void SetCurrentImageIndex(int imageIndex) { m_CurrentImageIndex = imageIndex; }
	void BeginRenderPass(VkRenderPass& renderPass);
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
