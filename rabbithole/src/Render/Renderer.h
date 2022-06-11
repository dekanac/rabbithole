#pragma once
#include "common.h"
#include "Logger/Logger.h"

#include "Vulkan/Include/VulkanWrapper.h"
#include "SuperResolutionManager.h"
#include "Window.h"
#include "Model/Model.h"
#include "BVH.h"

#include <unordered_map>
#include <string>
#include <optional>

//#ifdef _DEBUG
	//#define  USE_RABBITHOLE_TOOLS
	#define RABBITHOLE_USING_IMGUI
//#endif
#define MAX_NUM_OF_LIGHTS 4
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
	float power;
	int kernelSize;
	bool ssaoOn;
};

struct IndexedIndirectBuffer
{
    VulkanBuffer* gpuBuffer = nullptr;
    IndexIndirectDrawData* localBuffer = nullptr;

	uint64_t currentSize = 0;
	uint64_t currentOffset = 0;

	void SubmitToGPU();
	void AddIndirectDrawCommand(VkCommandBuffer commandBuffer, IndexIndirectDrawData& drawData);
	void Reset();
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

	VulkanBuffer* m_MainConstBuffer[MAX_FRAMES_IN_FLIGHT];
	VulkanBuffer* m_VertexUploadBuffer;
	
	//test purpose only
	Entity*						testEntity;
	
	void LoadModels();
	void LoadAndCreateShaders();
	void CreateShaderModule(const std::vector<char>& code, ShaderType type, const char* name, const char* codeEntry);
	void CreateCommandBuffers();
	void RecreateSwapchain();
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

	void ResourceBarrier(VulkanTexture* texture, ResourceState oldLayout, ResourceState newLayout, ResourceStage srcStage, ResourceStage dstStage);
	void CopyImageToBuffer(VulkanTexture* texture, VulkanBuffer* buffer);
	void CopyImage(VulkanTexture* src, VulkanTexture* dst);

	inline Shader* GetShader(const std::string& name) { return m_Shaders[name]; }
	inline Camera* GetCamera() { return MainCamera; }

	inline VulkanBuffer* GetMainConstBuffer() { return m_MainConstBuffer[m_CurrentImageIndex]; }
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
	int GetCurrentImageIndex() { return m_CurrentImageIndex; }
	
	void BeginRenderPass(VkExtent2D extent);
	void EndRenderPass();

	void BindGraphicsPipeline(bool isCopyToSwapChain = false);
	void BindComputePipeline();
	void BindDescriptorSets();
	void BindUBO();
	VkCommandBuffer GetCurrentCommandBuffer() { return m_CommandBuffers[m_CurrentImageIndex]; }

	void BindCameraMatrices(Camera* camera);

	void DrawGeometryGLTF(std::vector<VulkanglTFModel>& bucket);
	//void DrawBoundingBoxes(std::vector<RabbitModel*>& bucket);
	void DrawFullScreenQuad();
	void UpdateEntityPickId();

	void BeginCommandBuffer();
	void EndCommandBuffer();

	//helper functions
	std::vector<char> ReadFile(const std::string& filepath);
	void FillTheLightParam(LightParams& lightParam, rabbitVec4f position, rabbitVec3f color, float radius);
	void CopyToSwapChain();
	void ExecuteRenderPass(RenderPass& renderpass);

public:
	std::vector<VulkanglTFModel> gltfModels;

	//default textures;
	VulkanTexture* g_DefaultWhiteTexture;
	VulkanTexture* g_DefaultBlackTexture;
	//TODO: do something with these
	VulkanTexture* depthStencil;
	//geometry
	IndexedIndirectBuffer* geomDataIndirectDraw;

	//gbuffer
	VulkanTexture* albedoGBuffer;
	VulkanTexture* normalGBuffer;
	VulkanTexture* worldPositionGBuffer;
	VulkanTexture* velocityGBuffer;
	
	//main lighting
	VulkanTexture* lightingMain;
	LightParams lightParams[numOfLights];
	VulkanBuffer* m_LightParams;
	
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
	VulkanTexture* historyBuffer[MAX_FRAMES_IN_FLIGHT];

	bool m_RenderOutlinedEntity = false;
    bool m_FramebufferResized = false;
	bool m_RenderTAA = true;
	bool m_DrawBoundingBox = false;

	bool Init();
	bool Shutdown();
	void Clear() const;
    void Draw(float dt);
    void DrawFrame();

private:
	void CreateGeometryDescriptors(std::vector<VulkanglTFModel>& models, uint32_t imageIndex);

	void InitImgui();
	bool m_ImguiInitialized = false;
public:
	//Don't ask, Imgui init wants swapchain renderpass to be ready, but its not. So basically we need 2 init phases..
	bool imguiReady = false;

private:
	void InitTextures();
};
