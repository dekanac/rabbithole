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
#define RABBITHOLE_USING_IMGUI
#define MAX_NUM_OF_LIGHTS 12

class Camera;
class EntityManager;
struct GLFWwindow;
struct Vertex;
class VulkanDevice;
class VulkanStateManager;
class Entity;
class Shader;
class RenderPass;

struct UniformBufferObject
{
	rabbitMat4f view;
	rabbitMat4f proj;
	rabbitVec3f cameraPos;
	rabbitVec3f debugOption;
};

struct LightParams
{
	rabbitVec4f position;
	rabbitVec4f colorAndRadius; //4th element of vector is radius
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
	
	VulkanBuffer*							m_UniformBuffer;
	VulkanBuffer*							m_LightParams;

	VkRenderPass							m_ImguiRenderPass;

	VulkanStateManager*						m_StateManager;

	int										m_CurrentImageIndex = 0;

	Entity*												testEntity;
	std::vector<RabbitModel*>			rabbitmodels;
	
	void loadModels();
	void LoadAndCreateShaders();
	void CreateShaderModule(const std::vector<char>& code, ShaderType type, const char* name, const char* codeEntry);
	void createCommandBuffers();
	void recreateSwapchain();
	void RecordCommandBuffer(int imageIndex);
	void CreateUniformBuffers();
	void CreateDescriptorPool();

public:
	inline VulkanDevice& GetVulkanDevice() { return m_VulkanDevice; }
	inline VulkanStateManager* GetStateManager() { return m_StateManager; }
	inline VulkanSwapchain* GetSwapchain() { return m_VulkanSwapchain.get(); }
	inline VulkanImageView* GetSwapchainImage() { return m_VulkanSwapchain->GetImageView(m_CurrentImageIndex); }

	void ResourceBarrier(VulkanTexture* texture, ResourceState oldLayout, ResourceState newLayout);

	inline Shader* GetShader(int index) { return m_Shaders[index]; }
	inline std::vector<RabbitModel*> GetModels() { return rabbitmodels; }
	inline Camera* GetCamera() { return MainCamera; }

	inline VulkanBuffer* GetUniformBuffer() { return m_UniformBuffer; }
	inline VulkanBuffer* GetLightParams() { return m_LightParams; }
	

	void UpdateDebugOptions();
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

	void DrawGeometry(std::vector<RabbitModel*> bucket);

	void BeginCommandBuffer();
	void EndCommandBuffer();

	//helper functions
	std::vector<char> ReadFile(const std::string& filepath);
	void DrawFullScreenQuad();
	void CopyToSwapChain();
	void ImageTransitionToPresent();
public:

	VulkanTexture* albedoGBuffer;
	VulkanTexture* normalGBuffer;
	VulkanTexture* worldPositionGBuffer;
	VulkanTexture* lightingMain;

    bool m_FramebufferResized = false;

	bool Init();
	bool Shutdown();
	void Clear() const;
    void Draw(float dt);
    void DrawFrame();

private:
	void CreateGeometryDescriptors();
	void InitImgui();

private:

	bool m_ImguiInitialized = false;
};
