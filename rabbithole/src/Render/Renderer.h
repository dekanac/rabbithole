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

#define DYNAMIC_SCISSOR_AND_VIEWPORT_STATES

class Camera;
class EntityManager;
struct GLFWwindow;
struct Vertex;
class VulkanDevice;
class VulkanStateManager;
class Entity;
class Shader;

struct UniformBufferObject
{
	rabbitMat4f view;
	rabbitMat4f proj;
	rabbitVec3f cameraPos;
	rabbitVec3f debugOption;
};

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
	
	VulkanDescriptorSet*					m_DescriptorSet;
	VulkanBuffer*							m_UniformBuffer;

	VulkanPipeline*							m_CurrentGraphicsPipeline;
	VulkanRenderPass*						m_CurrentRenderPass;
	VulkanStateManager*						m_StateManager;

	int										m_CurrentImageIndex = 0;

	Entity*												testEntity;
	ModelLoading::SceneData*							testScene;
	std::vector<RabbitModel*>			rabbitmodels;
	
	void loadModels();
	void LoadAndCreateShaders();
	void CreateShaderModule(const std::vector<char>& code, ShaderType type, const char* name, const char* codeEntry);
	void CreateRenderPasses();
	void CreateMainPhongLightingPipeline();
	void createCommandBuffers();
	void recreateSwapchain();
	void UpdateDebugOptions();
	void RecordCommandBuffer(int imageIndex);
	void UpdateUniformBuffer(uint32_t currentImage);
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();

	void BindViewport(float x, float y, float width, float height);

	template <typename T>
	void BindPushConstant(ShaderType shaderType, T& push)
	{
		vkCmdPushConstants(m_CommandBuffers[m_CurrentImageIndex], 
			*(m_StateManager->GetPipeline()->GetPipelineLayout()), 
			GetVkShaderStageFrom(shaderType), 
			0, 
			sizeof(T), 
			&push);
	}

	void SetCurrentImageIndex(int imageIndex) { m_CurrentImageIndex = imageIndex; }
	void BeginRenderPass();
	void EndRenderPass();

	void BindGraphicsPipeline();
	void BindDescriptorSets();
	void BindUBO();

	void BindModelMatrix(RabbitModel* model);
	void BindCameraMatrices(Camera* camera);

	void DrawBucket(std::vector<RabbitModel*> bucket);

	void BeginCommandBuffer();
	void EndCommandBuffer();

	//helper functions
	std::vector<char> ReadFile(const std::string& filepath);
	void PresentSwapchain();
public:
    bool m_FramebufferResized = false;

	bool Init();
	bool Shutdown();
	void Clear() const;
    void Draw(float dt);
    void DrawFrame();

};
