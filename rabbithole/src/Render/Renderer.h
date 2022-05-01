#pragma once
#include "common.h"
#include "Logger/Logger.h"

#include <unordered_map>
#include <string>
#include <vulkan/vulkan.h>
#include <optional>

#include "Vulkan/Include/VulkanWrapper.h"
#include "SuperResolutionManager.h"
#include "Window.h"
#include "Model/ModelLoading.h"

#ifdef _DEBUG
	#define RABBITHOLE_USING_IMGUI
#endif
#define MAX_NUM_OF_LIGHTS 12
constexpr size_t numOfLights = 4;

class Camera;
class EntityManager;
struct GLFWwindow;
struct Vertex;
class VulkanDevice;
class VulkanStateManager;
class Entity;
class Shader;
class RenderPass;

enum SimpleMeshType
{
	BoxMesh,
	SphereMesh,

	Count
};

struct UniformBufferObject
{
	rabbitMat4f view;
	rabbitMat4f proj;
	rabbitVec4f cameraPos;
	rabbitVec4f debugOption;
	rabbitMat4f viewProjInverse;
	rabbitMat4f viewProjMatrix;
	rabbitMat4f prevViewProjMatrix;
};

struct SSAOSamples
{
	rabbitVec3f samples[64];
};

struct PushMousePos
{
	uint32_t x;
	uint32_t y;
};

struct LightParams
{
	float position[4];
	float color[3];
	float radius;
};

struct SSAOParams
{
	float radius;
	float bias;
	float resWidth;
	float resHeight;
	int kernelSize;
	bool ssaoOn;
};

class Renderer
{
    SingletonClass(Renderer)

private:
	Camera* MainCamera{};

	VulkanDevice								m_VulkanDevice{};
	VulkanStateManager*							m_StateManager;
	std::unique_ptr<VulkanSwapchain>			m_VulkanSwapchain;
	std::unique_ptr<VulkanDescriptorPool>		m_DescriptorPool;
	std::unordered_map<std::string, Shader*>	m_Shaders;
	std::vector<VkCommandBuffer>				m_CommandBuffers;
	uint8_t										m_CurrentImageIndex = 0;

	VulkanBuffer* m_MainConstBuffer;
	VulkanBuffer* m_VertexUploadBuffer;
	VulkanBuffer* m_LightParams;
	
	//test purpose only
	Entity*						testEntity;
	ModelLoading::SceneData* defaultBoxModel;
	ModelLoading::SceneData* defaultSphereModel;
	std::vector<RabbitModel*>	rabbitmodels;
	
	void loadModels();
	void LoadAndCreateShaders();
	void CreateShaderModule(const std::vector<char>& code, ShaderType type, const char* name, const char* codeEntry);
	void createCommandBuffers();
	void recreateSwapchain();
	void RecordCommandBuffer(int imageIndex);
	void CreateUniformBuffers();
	void CreateDescriptorPool();

	void InitSSAO();
	void InitLights();
	void InitMeshDataForCompute();
public:
	inline VulkanDevice& GetVulkanDevice() { return m_VulkanDevice; }
	inline VulkanStateManager* GetStateManager() { return m_StateManager; }
	inline VulkanSwapchain* GetSwapchain() { return m_VulkanSwapchain.get(); }
	inline VulkanImageView* GetSwapchainImage() { return m_VulkanSwapchain->GetImageView(m_CurrentImageIndex); }
	inline VulkanBuffer* GetVertexUploadBuffer() { return m_VertexUploadBuffer; }

	void ResourceBarrier(VulkanTexture* texture, ResourceState oldLayout, ResourceState newLayout);
	void CopyImageToBuffer(VulkanTexture* texture, VulkanBuffer* buffer);
	void CopyImage(VulkanTexture* src, VulkanTexture* dst);

	inline Shader* GetShader(const std::string& name) { return m_Shaders[name]; }
	inline std::vector<RabbitModel*>& GetModels() { return rabbitmodels; }
	inline Camera* GetCamera() { return MainCamera; }

	inline VulkanBuffer* GetMainConstBuffer() { return m_MainConstBuffer; }
	inline VulkanBuffer* GetLightParams() { return m_LightParams; }

	void UpdateDebugOptions();
	void BindViewport(float x, float y, float width, float height);
	void BindVertexData(size_t offset);

	void DrawVertices(uint64_t count);
	void DrawIndicesIndirect(uint32_t count, uint32_t offset);
	void Dispatch(uint32_t x, uint32_t y, uint32_t z);

	template <typename T>
	void BindPushConstant(T& push)
	{
		vkCmdPushConstants(GetCurrentCommandBuffer(), 
			*(m_StateManager->GetPipeline()->GetPipelineLayout()), 
			VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, //TODO: HC
			0, 
			sizeof(T), 
			&push);
	}

	void SetCurrentImageIndex(int imageIndex) { m_CurrentImageIndex = imageIndex; }
	
	void BeginRenderPass(VkExtent2D extent);
	void EndRenderPass();

	void BindGraphicsPipeline(bool isCopyToSwapChain = false);
	void BindComputePipeline();
	void BindDescriptorSets();
	void BindUBO();
	VkCommandBuffer GetCurrentCommandBuffer() { return m_CommandBuffers[m_CurrentImageIndex]; }

	void BindModelMatrix(RabbitModel* model);
	void BindCameraMatrices(Camera* camera);

	void DrawGeometry(std::vector<RabbitModel*>& bucket);
	void DrawBoundingBoxes(std::vector<RabbitModel*>& bucket);
	void DrawFullScreenQuad();
	void UpdateEntityPickId();

	void BeginCommandBuffer();
	void EndCommandBuffer();

	//helper functions
	std::vector<char> ReadFile(const std::string& filepath);
	void FillTheLightParam(LightParams& lightParam, rabbitVec4f position, rabbitVec3f color, float radius);
	void CopyToSwapChain();
	void ImageTransitionToPresent();
	void ExecuteRenderPass(RenderPass& renderpass);

	void AddSimpleMesh(SimpleMeshType type, rabbitVec3f position = rabbitVec3f{}, float size = 1.f, rabbitVec3f rotation = rabbitVec3f{});
public:

	//TODO: do something with these
	VulkanTexture* depthStencil;

	//geometry
	VulkanBuffer* geomDataIndirectDraw;
	IndexIndirectDrawData* indexedDataBuffer;

	//gbuffer
	VulkanTexture* albedoGBuffer;
	VulkanTexture* normalGBuffer;
	VulkanTexture* worldPositionGBuffer;
	VulkanTexture* velocityGBuffer;
	
	//main lighting
	VulkanTexture* lightingMain;
	LightParams lightParams[numOfLights];
	
	//skybox
	VulkanTexture* skyboxTexture;

	//ssao
	VulkanTexture* SSAOTexture;
	VulkanTexture* SSAOBluredTexture;
	VulkanTexture* SSAONoiseTexture;
	VulkanBuffer*  SSAOSamplesBuffer;
	VulkanBuffer*  SSAOParamsBuffer;
	SSAOParams	   ssaoParams{};
	
	//entity helper
	VulkanTexture* entityHelper;
	VulkanTexture* DebugTextureRT;
	VulkanBuffer*  entityHelperBuffer;
	
	//RT shadows helper
	VulkanBuffer* vertexBuffer;
	VulkanBuffer* trianglesBuffer;
	VulkanBuffer* triangleIndxsBuffer;
	VulkanBuffer* cfbvhNodesBuffer;
	VulkanTexture* shadowMap;

	//FSR
	VulkanBuffer* fsrParamsBuffer;
	VulkanTexture* fsrOutputTexture;
	VulkanTexture* fsrIntermediateRes;

	//TAA
	VulkanTexture* TAAOutput;
	VulkanTexture* historyBuffer;

	bool m_RenderOutlinedEntity = false;
    bool m_FramebufferResized = false;

	bool m_DrawBoundingBox = false;

	bool Init();
	bool Shutdown();
	void Clear() const;
    void Draw(float dt);
    void DrawFrame();

private:
	void CreateGeometryDescriptors();
	void InitImgui();
	bool m_ImguiInitialized = false;
public:
	//Don't ask, Imgui init wants swapchain renderpass to be ready, but its not. So basically we need 2 init phases..
	bool imguiReady = false;

private:
	void InitTextures();
};
