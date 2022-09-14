#pragma once
#include "common.h"

#include "Logger/Logger.h"
#include "Render/BVH.h"
#include "Render/Camera.h"
#include "Render/Model/Model.h"
#include "Render/PipelineManager.h"
#include "Render/ResourceManager.h"
#include "Render/ResourceStateTracking.h"
#include "Render/SuperResolutionManager.h"
#include "Render/Vulkan/Include/VulkanWrapper.h"
#include "Render/Window.h"

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
class Entity;
class EntityManager;
class RabbitPass;
class Shader;
class VulkanDevice;
class VulkanStateManager;
struct CameraState;
struct GLFWwindow;
struct Vertex;

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
	uint32_t type;
	float size;
	uint32_t padding[2];
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
	void AddIndirectDrawCommand(VulkanCommandBuffer& commandBuffer, IndexIndirectDrawData& drawData);
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
	SingletonClass(Renderer);

private:
	VulkanDevice										m_VulkanDevice{};
	VulkanStateManager*									m_StateManager;
	ResourceManager*									m_ResourceManager;
	std::unique_ptr<VulkanSwapchain>					m_VulkanSwapchain;
	std::unique_ptr<VulkanDescriptorPool>				m_DescriptorPool;
	std::vector<std::unique_ptr<VulkanCommandBuffer>>	m_MainRenderCommandBuffers;
	uint32_t											m_CurrentImageIndex = 0;
	uint64_t											m_CurrentFrameIndex = 0;

	VulkanBuffer* m_MainConstBuffer[MAX_FRAMES_IN_FLIGHT];
	VulkanBuffer* m_VertexUploadBuffer;
	
	Camera			m_MainCamera{};
	CameraState		m_CurrentCameraState{};
	UIState			m_CurrentUIState{};
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

	ImVec2 GetScaledSizeWithAspectRatioKept(ImVec2 currentSize);
public:
	inline VulkanDevice&		GetVulkanDevice() { return m_VulkanDevice; }
	inline VulkanStateManager*	GetStateManager() const { return m_StateManager; }
	inline VulkanSwapchain*		GetSwapchain() const { return m_VulkanSwapchain.get(); }
	inline ResourceManager*		GetResourceManager() const { return m_ResourceManager; }
	inline VulkanImageView*		GetSwapchainImage() { return m_VulkanSwapchain->GetImageView(m_CurrentImageIndex); }
	inline VulkanBuffer*		GetVertexUploadBuffer() { return m_VertexUploadBuffer; }

	void ResourceBarrier(VulkanTexture* texture, ResourceState oldLayout, ResourceState newLayout, ResourceStage srcStage, ResourceStage dstStage, uint32_t mipLevel = 0);
    void CopyImageToBuffer(VulkanTexture* texture, VulkanBuffer* buffer);
    void CopyBufferToImage( VulkanBuffer* buffer, VulkanTexture* texture);
	void CopyImage(VulkanTexture* src, VulkanTexture* dst);
	void CopyBuffer(VulkanBuffer& src, VulkanBuffer& dst, uint64_t size = UINT64_MAX, uint64_t srcOffset = 0, uint64_t dstOffset = 0);

	inline Shader*			GetShader(const std::string& name) const { return m_ResourceManager->GetShader(name); }
	inline VulkanTexture*	GetTextureWithID(uint32_t textureId) { return m_ResourceManager->GetTextures()[textureId]; }
	inline Camera&			GetCamera() { return m_MainCamera; }
	inline UIState&			GetUIState() { return m_CurrentUIState; }
	inline CameraState&		GetCameraState() { return m_CurrentCameraState; }

	inline VulkanBuffer* GetMainConstBuffer() { return m_MainConstBuffer[m_CurrentImageIndex]; }
	inline VulkanBuffer* GetLightParams() { return m_LightParams; }

	void UpdateEntityPickId();
	void BindViewport(float x, float y, float width, float height);
	void BindVertexData(size_t offset);

	void DrawVertices(uint32_t count);
	void Dispatch(uint32_t x, uint32_t y, uint32_t z);
	
	void CopyToSwapChain();
	void DrawGeometryGLTF(std::vector<VulkanglTFModel>& bucket);
	void DrawFullScreenQuad();

	template <typename T>
	void BindPushConstant(T&& push)
	{
		PushConstant pushConst(reinterpret_cast<void*>(push), static_cast<uint32_t>(sizeof(T)));

		m_StateManager->SetPushConst(pushConst);
	}

	void BindPushConstInternal();

	void		SetCurrentImageIndex(int imageIndex) { m_CurrentImageIndex = imageIndex; }
	uint32_t	GetCurrentImageIndex() { return m_CurrentImageIndex; }
	uint64_t	GetCurrentFrameIndex() { return m_CurrentFrameIndex; }
	
	VulkanCommandBuffer& GetCurrentCommandBuffer() { return *m_MainRenderCommandBuffers[m_CurrentImageIndex]; }
	
	void RecordCommandBuffer();
	void BeginRenderPass(VkExtent2D extent);
	void EndRenderPass();

	void BindCameraMatrices(Camera* camera);
	
	template<class T = Pipeline> void BindPipeline();
	template<> void BindPipeline<GraphicsPipeline>();
	template<> void BindPipeline<ComputePipeline>();

	void BindDescriptorSets();
	void BindUBO();

	void RecordGPUTimeStamp(const char* label);
	void ExecuteRabbitPass(RabbitPass& rabbitPass);

	//helper functions
	std::vector<char>	ReadFile(const std::string& filepath);
	void				FillTheLightParam(LightParams& lightParam, rabbitVec3f position, rabbitVec3f color, float radius, float intensity, LightType type, float size);

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
	VkDescriptorSet finalImageImGuiDS;
	
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
	VulkanBuffer* denoiseShadowMaskBuffer[MAX_NUM_OF_LIGHTS];
	VulkanBuffer* denoiseBufferDimensions;
	VulkanBuffer* denoiseTileMetadataBuffer[MAX_NUM_OF_LIGHTS];
	VulkanTexture* denoiseMomentsBuffer0[MAX_NUM_OF_LIGHTS];
	VulkanTexture* denoiseMomentsBuffer1[MAX_NUM_OF_LIGHTS];
	VulkanTexture* denoiseReprojectionBuffer0[MAX_NUM_OF_LIGHTS];
	VulkanTexture* denoiseReprojectionBuffer1[MAX_NUM_OF_LIGHTS];
	VulkanTexture* denoiseLastFrameDepth;
	VulkanBuffer* denoiseShadowDataBuffer;
	VulkanBuffer* denoiseShadowFilterDataBuffer;
	VulkanTexture* denoisedShadowOutput;

	bool m_RenderOutlinedEntity = false;
    bool m_FramebufferResized = false;
	bool m_RenderTAA = false;
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
	void DestroyImgui();
	bool m_ImguiInitialized = false;
	float m_CurrentDeltaTime;

public:
	//Don't ask, Imgui init wants swapchain renderpass to be ready, but its not. So basically we need 2 init phases..
	bool imguiReady = false;
#ifdef RABBITHOLE_USING_IMGUI
	bool isInEditorMode = true;
#else
	bool isInEditorMode = false;
#endif
};
