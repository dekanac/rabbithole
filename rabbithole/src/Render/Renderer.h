#pragma once
#include "common.h"
#include "Logger/Logger.h"

#include "Vulkan/Include/VulkanWrapper.h"
#include "SuperResolutionManager.h"
#include "Window.h"
#include "Model/Model.h"
#include "BVH.h"
#include "Render/ResourceManager.h"
#include "Render/ResourceStateTracking.h"
#include "PipelineManager.h"

#include <unordered_map>
#include <string>
#include <optional>

//#ifdef _DEBUG
	//#define  USE_RABBITHOLE_TOOLS
	#define RABBITHOLE_USING_IMGUI
//#endif
#define MAX_NUM_OF_LIGHTS 4
constexpr size_t numOfLights = MAX_NUM_OF_LIGHTS;

class Camera;
struct CameraState;
class EntityManager;
struct GLFWwindow;
struct Vertex;
class VulkanDevice;
class VulkanStateManager;
class Entity;
class Shader;
class RabbitPass;

typedef VkExtent2D Extent2D;

enum SimpleMeshType
{
	BoxMesh,
	SphereMesh,

	Count
};

struct UIState
{
	float renderWidth;
	float renderHeight;
	bool reset;
	bool useRcas;
	float sharpness;
	float deltaTime;
	bool useTaa;
	Camera* camera;
};

//keep in sync with UBOElement in VulkanStatemanager
struct UniformBufferObject
{
	rabbitMat4f view;
	rabbitMat4f proj;
	rabbitVec4f cameraPos;
	rabbitVec4f debugOption;
	rabbitMat4f viewProjInverse;
	rabbitMat4f viewProjMatrix;
	rabbitMat4f prevViewProjMatrix;
	rabbitMat4f viewInverse;
	rabbitMat4f projInverse;
    rabbitVec4f frustrumInfo;
	rabbitVec4f eyeXAxis;
	rabbitVec4f eyeYAxis;
	rabbitVec4f eyeZAxis;
	rabbitMat4f projJittered;
	rabbitVec4f currentFrameInfo;
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
	float position[3];
	float radius;
	float color[3];
	float intensity;
	alignas(16) uint32_t type;
};

enum LightType : uint32_t
{
	LightType_Directional = 0,
	LightType_Point = 1,
	LightType_Spot = 2
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

struct DebugTextureParams
{
	uint32_t hasMips = false;
	int mipSlice = 0;
	int mipCount = 1;

	uint32_t isArray = false;
	int arraySlice = 0;
	int arrayCount = 1;

	uint32_t showR = true;
	uint32_t showG = true;
	uint32_t showB = true;
	uint32_t showA = true;

	bool is3D;
	float texture3DDepthScale = 0.f;
};

struct VolumetricFogParams
{
	uint32_t isEnabled = true;
	float fogAmount = 0.006f;
	float depthScale_debug = 2.f;
	float fogStartDistance = 0.1f;
	float fogDistance = 64.f;
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

struct DenoiseBufferDimensions
{
	uint32_t dimensions[2];
};

struct DenoiseShadowData
{
	rabbitVec3f		Eye;
	int				FirstFrame;
	int32_t			BufferDimensions[2];
	float			InvBufferDimensions[2];
	rabbitMat4f		ProjectionInverse;
	rabbitMat4f		ReprojectionMatrix;
	rabbitMat4f		ViewProjectionInverse;
};

struct DenoiseShadowFilterData
{
	rabbitMat4f ProjectionInverse;
	int32_t     BufferDimensions[2];
	float		InvBufferDimensions[2];
	float		DepthSimilaritySigma;
};

class Renderer
{
	SingletonClass(Renderer)

private:
	VulkanDevice								m_VulkanDevice{};
	VulkanStateManager*							m_StateManager;
	ResourceManager*							m_ResourceManager;
	std::unique_ptr<VulkanSwapchain>			m_VulkanSwapchain;
	std::unique_ptr<VulkanDescriptorPool>		m_DescriptorPool;
	std::vector<VkCommandBuffer>				m_CommandBuffers;
	uint8_t										m_CurrentImageIndex = 0;
	uint64_t									m_CurrentFrameIndex = 0;

	VulkanBuffer* m_MainConstBuffer[MAX_FRAMES_IN_FLIGHT];
	VulkanBuffer* m_VertexUploadBuffer;
	
	Camera*			MainCamera{};
	CameraState*	m_CurrentCameraState{};
	UIState*		m_CurrentUIState{};
	GPUTimeStamps	m_GPUTimeStamps{};
	
	void LoadModels();
	void LoadAndCreateShaders();
	void CreateCommandBuffers();
	void RecreateSwapchain();
	void CreateUniformBuffers();
	void CreateDescriptorPool();

	void InitSSAO();
	void InitLights();
	void InitMeshDataForCompute();
	void UpdateConstantBuffer();
	void UpdateUIStateAndFSR2PreDraw();
	void ImguiProfilerWindow(std::vector<TimeStamp>& timestamps);
	void RegisterTexturesToImGui();
	void ImGuiTextureDebugger();

public:
	inline VulkanDevice&		GetVulkanDevice() { return m_VulkanDevice; }
	inline VulkanStateManager*	GetStateManager() const { return m_StateManager; }
	inline VulkanSwapchain*		GetSwapchain() const { return m_VulkanSwapchain.get(); }
	inline ResourceManager*		GetResourceManager() const { return m_ResourceManager; }
	inline VulkanImageView*		GetSwapchainImage() { return m_VulkanSwapchain->GetImageView(m_CurrentImageIndex); }
	inline VulkanBuffer*		GetVertexUploadBuffer() { return m_VertexUploadBuffer; }

	void ResourceBarrier(VulkanTexture* texture, ResourceState oldLayout, ResourceState newLayout, ResourceStage srcStage, ResourceStage dstStage);
	void CopyImageToBuffer(VulkanTexture* texture, VulkanBuffer* buffer);
	void CopyImage(VulkanTexture* src, VulkanTexture* dst);

	inline Shader*			GetShader(const std::string& name) { return m_ResourceManager->GetShader(name); }
	inline VulkanTexture*	GetTextureWithID(uint32_t textureId) { return m_ResourceManager->GetTextures()[textureId]; }
	inline Camera*			GetCamera() { return MainCamera; }
	inline UIState*			GetUIState() { return m_CurrentUIState; }
	inline CameraState*		GetCameraState() const { return m_CurrentCameraState; }

	inline VulkanBuffer* GetMainConstBuffer() { return m_MainConstBuffer[m_CurrentImageIndex]; }
	inline VulkanBuffer* GetLightParams() { return m_LightParams; }

	void UpdateDebugOptions();
	void UpdateEntityPickId();
	void BindViewport(float x, float y, float width, float height);
	void BindVertexData(size_t offset);

	void DrawVertices(uint32_t count);
	void DrawIndicesIndirect(uint32_t count, uint32_t offset);
	void Dispatch(uint32_t x, uint32_t y, uint32_t z);
	
	void CopyToSwapChain();
	void DrawGeometryGLTF(std::vector<VulkanglTFModel>& bucket);
	void DrawFullScreenQuad();

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

	void		SetCurrentImageIndex(int imageIndex) { m_CurrentImageIndex = imageIndex; }
	int			GetCurrentImageIndex() { return m_CurrentImageIndex; }
	uint64_t	GetCurrentFrameIndex() { return m_CurrentFrameIndex; }
	
	VkCommandBuffer GetCurrentCommandBuffer() { return m_CommandBuffers[m_CurrentImageIndex]; }
	void RecordCommandBuffer(int imageIndex);
	
	void BeginRenderPass(VkExtent2D extent);
	void EndRenderPass();

	void BindCameraMatrices(Camera* camera);
	
	template<class T = Pipeline> void BindPipeline();
	template<> void BindPipeline<GraphicsPipeline>();
	template<> void BindPipeline<ComputePipeline>();

	void BindDescriptorSets();
	void BindUBO();

	//void DrawBoundingBoxes(std::vector<RabbitModel*>& bucket);

	void BeginCommandBuffer();
	void RecordGPUTimeStamp(const char* label);
	void ExecuteRabbitPass(RabbitPass& rabbitPass);
	void EndCommandBuffer();

	//helper functions
	std::vector<char>	ReadFile(const std::string& filepath);
	void				FillTheLightParam(LightParams& lightParam, rabbitVec3f position, rabbitVec3f color, float radius, float intensity, LightType type);

public:
	std::vector<VulkanglTFModel> gltfModels;

	//default textures;
	VulkanTexture* g_DefaultWhiteTexture;
	VulkanTexture* g_DefaultBlackTexture;
	VulkanTexture* g_Default3DTexture;
	VulkanTexture* g_DefaultArrayTexture;
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
	VulkanBuffer*  entityHelperBuffer;

	//debug texture
	VulkanTexture* debugTexture;
	VulkanBuffer* debugTextureParamsBuffer;
	DebugTextureParams debugTextureParams{};
	std::string currentTextureSelectedName = "Choose texture to debug: ";
	uint32_t currentTextureSelectedID;
	VkDescriptorSet debugTextureImGuiDS;
	
	//RT shadows helper
	VulkanBuffer* vertexBuffer;
	VulkanBuffer* trianglesBuffer;
	VulkanBuffer* triangleIndxsBuffer;
	VulkanBuffer* cfbvhNodesBuffer;
	VulkanTexture* shadowMap;

	//FSR
	VulkanTexture* fsrOutputTexture;

	//frustrum 3d map
	VulkanTexture* volumetricOutput;
	VulkanTexture* mediaDensity3DLUT;
	VulkanTexture* scatteringTexture;
	VulkanTexture* noise3DLUT;
	VulkanTexture* noise2DTexture;
	VulkanTexture* blueNoise2DTexture;
	VulkanBuffer* volumetricFogParamsBuffer;
	VolumetricFogParams volumetricFogParams{};
	bool init3dnoise = false;

	//posteffects
	VulkanTexture* postUpscalePostEffects;

	//shadow denoise
	VulkanBuffer* denoiseShadowMaskBuffer;
	VulkanBuffer* denoiseBufferDimensions;
	VulkanBuffer* denoiseTileMetadataBuffer;
	VulkanTexture* denoiseMomentsBuffer0;
	VulkanTexture* denoiseMomentsBuffer1;
	VulkanTexture* denoiseReprojectionBuffer0;
	VulkanTexture* denoiseReprojectionBuffer1;
	VulkanTexture* denoiseLastFrameDepth;
	VulkanBuffer* denoiseShadowDataBuffer;
	VulkanBuffer* denoiseShadowFilterDataBuffer;
	VulkanTexture* denoisedShadowOutput;

	bool m_RenderOutlinedEntity = false;
    bool m_FramebufferResized = false;
	bool m_RenderTAA = false;
	bool m_DrawBoundingBox = false;
	bool m_RecordGPUTimeStamps = true;

	bool Init();
	bool Shutdown();
	void Clear() const;
    void Draw(float dt);
    void DrawFrame();

private:
	void CreateGeometryDescriptors(std::vector<VulkanglTFModel>& models, uint32_t imageIndex);
	void InitTextures();
	void InitNoiseTextures();
	void InitImgui();
	bool m_ImguiInitialized = false;
	float m_CurrentDeltaTime;

public:
	//Don't ask, Imgui init wants swapchain renderpass to be ready, but its not. So basically we need 2 init phases..
	bool imguiReady = false;
};


#include "Renderer.hpp"