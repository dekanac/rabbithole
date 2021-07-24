#pragma once
#include "common.h"
#include "Logger/Logger.h"

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

class Renderer
{
    SingletonClass(Renderer)

private:
	Camera* MainCamera{};
	
	VulkanDevice m_VulkanDevice{};
	std::unique_ptr<VulkanSwapchain> m_VulkanSwapchain;
	std::unique_ptr<VulkanPipeline> m_VulkanPipeline;
	VkPipelineLayout pipelineLayout;
	std::vector<VkCommandBuffer> commandBuffers;
	std::unique_ptr<VulkanDescriptorPool> m_DescriptorPool;

	std::unique_ptr<RabbitModel> rabbitmodel;
	Entity* testEntity;

	void loadModels();
	void createPipelineLayout();
	void createPipeline();
	void createCommandBuffers();
	void recreateSwapchain();
	void recordCommandBuffer(int imageIndex);
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();
public:
    bool m_FramebufferResized = false;

	bool Init();
	bool Shutdown();
	void Clear() const;
    void Draw(float dt);
    void DrawFrame();

};
static std::vector<RabbitModel::Vertex> loadOBJ(const char* file_name);