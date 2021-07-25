#pragma once
#include "common.h"
#include "Logger/Logger.h"

#include <unordered_map>
#include <string>
#include <vulkan/vulkan.h>
#include <optional>

#include "Vulkan/Include/VulkanWrapper.h"
#include "Window.h"


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


	std::unique_ptr<RabbitModel>			rabbitmodel;
	Entity*									testEntity;

	void loadModels();
	void LoadAndCreateShaders();
	void CreateShaderModule(const std::vector<char>& code, ShaderType type, const char* name, const char* codeEntry);
	void createPipelineLayout();
	void createPipeline();
	void createCommandBuffers();
	void recreateSwapchain();
	void recordCommandBuffer(int imageIndex);
	void UpdateUniformBuffer(uint32_t currentImage);
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	
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
