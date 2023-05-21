#pragma once
#include "common.h"

#include "Logger/Logger.h"
#include "Render/BVH.h"
#include "Render/Camera.h"
#include "Render/ImGuiManager.h"
#include "Render/Model/Model.h"
#include "Render/PipelineManager.h"
#include "Render/Raytracing.h"
#include "Render/RenderPass.h"
#include "Render/ResourceManager.h"
#include "Render/ResourceStateTracking.h"
#include "Render/RabbitPassManager.h"
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

struct UIState
{
	float	renderWidth;
	float	renderHeight;
	bool	reset;
	bool	useRcas = true;
	float	sharpness = 1.f;
	float	deltaTime;
	bool	useTaa = true;
	Camera* camera;
};

struct LightParams
{
	float		position[3];
	float		radius;
	float		color[3];
	float		intensity;
	uint32_t	type;
	float		size;
	float		outerConeCos;
	float		innerConeCos;
};

enum LightType : uint32_t
{
	LightType_Directional = 0,
	LightType_Point = 1,
	LightType_Spot = 2
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

struct IndexedIndirectBuffer
{
	IndexedIndirectBuffer(VulkanDevice& device, uint32_t numCommands);
	~IndexedIndirectBuffer();

	VulkanBuffer gpuBuffer;
	IndexIndirectDrawData* localBuffer;

	uint64_t currentSize = 0;
	uint64_t currentOffset = 0;

	void SubmitToGPU();
	void AddIndirectDrawCommand(VulkanCommandBuffer& commandBuffer, IndexIndirectDrawData& drawData);
	void Reset();
};

class Renderer
{
	SingletonClass(Renderer);

private:
	VulkanDevice										m_VulkanDevice{};
	VulkanStateManager									m_StateManager{};
	ResourceStateTrackingManager						m_ResourceStateTrackingManager{};
	ResourceManager										m_ResourceManager{};
	RabbitPassManager									m_RabbitPassManager{};
	PipelineManager										m_PipelineManager{};
	ImGuiManager										m_ImGuiManager{};

	std::unique_ptr<VulkanSwapchain>					m_VulkanSwapchain;
	std::unique_ptr<VulkanDescriptorPool>				m_DescriptorPool;
	std::vector<std::unique_ptr<VulkanCommandBuffer>>	m_MainRenderCommandBuffers;
	uint32_t											m_CurrentImageIndex = 0;
	uint64_t											m_CurrentFrameIndex = 0;

	VulkanBuffer*	m_MainConstBuffer[MAX_FRAMES_IN_FLIGHT];
	VulkanBuffer*	m_VertexUploadBuffer;
	
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

	void InitLights();
	void ConstructBVH();
	void CreateAccelerationStructure();
	void UpdateConstantBuffer();
	void UpdateUIStateAndFSR2PreDraw();
	void ImguiProfilerWindow(std::vector<TimeStamp>& timestamps);
	void ImGuiTextureDebugger();
	ImVec2 GetScaledSizeWithAspectRatioKept(ImVec2 currentSize, float minWidth = 200.f);

	void RecordCommandBuffer();

	void BindPushConstInternal();
	template<class T = Pipeline> void BindPipeline();
	template<> void BindPipeline<GraphicsPipeline>();
	template<> void BindPipeline<ComputePipeline>();
	template<> void BindPipeline<RayTracingPipeline>();
	void BindCameraMatrices(Camera* camera);
	void BindDescriptorSets();
	void BindUBO();

	RayTracingPipeline* tmpRTPipeline;
public:
	inline VulkanDevice&					GetVulkanDevice() { return m_VulkanDevice; }
	inline VulkanStateManager&				GetStateManager() { return m_StateManager; }
	inline ResourceStateTrackingManager&	GetResourceStateTrackingManager() { return m_ResourceStateTrackingManager; }
	inline ResourceManager&					GetResourceManager() { return m_ResourceManager; }
	inline RabbitPassManager&				GetRabbitPassManager() { return m_RabbitPassManager; }
	inline PipelineManager&					GetPipelineManager() { return m_PipelineManager; }

	inline VulkanSwapchain*					GetSwapchain() const { return m_VulkanSwapchain.get(); }
	inline VulkanImageView*					GetSwapchainImage() { return m_VulkanSwapchain->GetImageView(m_CurrentImageIndex); }
	inline VulkanDescriptorPool&			GetDescriptorPool() { return *m_DescriptorPool; }
	inline VulkanBuffer*					GetVertexUploadBuffer() { return m_VertexUploadBuffer; }
	inline VulkanBuffer*					GetMainConstBuffer() { return m_MainConstBuffer[m_CurrentImageIndex]; }

	void ResourceBarrier(VulkanTexture* texture, ResourceState oldLayout, ResourceState newLayout, ResourceStage srcStage, ResourceStage dstStage, uint32_t mipLevel = 0, uint32_t mipCount = UINT32_MAX);
	void ResourceBarrier(VulkanBuffer* buffer, ResourceState oldLayout, ResourceState newLayout, ResourceStage srcStage, ResourceStage dstStage);
    void CopyImageToBuffer(VulkanTexture* texture, VulkanBuffer* buffer);
    void CopyBufferToImage( VulkanBuffer* buffer, VulkanTexture* texture);
	void CopyImage(VulkanTexture* src, VulkanTexture* dst);
	void CopyBuffer(VulkanBuffer& src, VulkanBuffer& dst, uint64_t size = UINT64_MAX, uint64_t srcOffset = 0, uint64_t dstOffset = 0);

	inline Shader*			GetShader(const std::string& name) { return m_ResourceManager.GetShader(name); }
	inline VulkanTexture*	GetTextureWithID(uint32_t textureId) { return m_ResourceManager.GetTextures()[textureId]; }
	inline Camera&			GetCamera() { return m_MainCamera; }
	inline UIState&			GetUIState() { return m_CurrentUIState; }
	inline CameraState&		GetCameraState() { return m_CurrentCameraState; }
	inline bool				IsImguiReady() { return m_ImGuiManager.IsInitialized() && m_ImGuiManager.IsReady(); }

	void UpdateEntityPickId();

	template<typename T>
	void BindPushConst(T& data)
	{
		auto size = sizeof(T);
		ASSERT(size < 128, "Max size of push const is 128!");
		PushConstant* pushConst = m_StateManager.GetPushConst();
		memcpy(pushConst->data, &data, size);
        pushConst->size = static_cast<uint32_t>(size);
		m_StateManager.SetShouldBindPushConst(true);
	}
	void BindViewport(float x, float y, float width, float height);
	void BindVertexData(size_t offset);
	void DrawVertices(uint32_t count);
	void Dispatch(uint32_t x, uint32_t y, uint32_t z);
	void CopyToSwapChain();
	void DrawGeometryGLTF(std::vector<VulkanglTFModel>& bucket);
	void DrawFullScreenQuad();
	void TraceRays(uint32_t x, uint32_t y, uint32_t z);
	void ClearImage(VulkanTexture* texture, Color clearValue);

	uint32_t	GetCurrentImageIndex() { return m_CurrentImageIndex; }
	uint64_t	GetCurrentFrameIndex() { return m_CurrentFrameIndex; }
	
	VulkanCommandBuffer& GetCurrentCommandBuffer() { return *m_MainRenderCommandBuffers[m_CurrentImageIndex]; }

	//debugging
	void RecordGPUTimeStamp(const char* label);
	void BeginLabel(const char* name);
	void EndLabel();

public:
	std::vector<VulkanglTFModel> gltfModels;
	std::vector<LightParams> lights;

	//default textures;
	VulkanTexture* g_DefaultWhiteTexture;
	VulkanTexture* g_DefaultBlackTexture;
	VulkanTexture* g_Default3DTexture;
	VulkanTexture* g_DefaultArrayTexture;

	//geometry
	IndexedIndirectBuffer* m_GeometryIndirectDrawBuffer;

	//debug texture
	std::string currentTextureSelectedName = "Choose texture to debug: ";
	uint32_t currentTextureSelectedID;
	
	//BVH Construction
	VulkanBuffer* vertexBuffer;
	VulkanBuffer* trianglesBuffer;
	VulkanBuffer* triangleIndxsBuffer;
	VulkanBuffer* cfbvhNodesBuffer;

#if defined(VULKAN_HWRT)
	RayTracing::AccelerationStructure BLAS;
	RayTracing::AccelerationStructure TLAS;
#endif

	//frustrum 3d map
	VulkanTexture* noise3DLUT;
	VulkanTexture* noise2DTexture;
	VulkanTexture* blueNoise2DTexture;

	bool m_RenderOutlinedEntity = false;
    bool m_FramebufferResized = false;
	bool m_RenderTAA = false;
	bool m_RecordGPUTimeStamps = true;
	uint32_t m_TriangleCount = 0;

	bool Init();
	bool Shutdown();
	void Clear() const;
    void Draw(float dt);
    void DrawFrame();

private:
	void CreateGeometryDescriptors(std::vector<VulkanglTFModel>& models, uint32_t imageIndex);
	void InitDefaultTextures();
	float m_CurrentDeltaTime;
	float m_CurrentCPUTimeInMS;

public:
	//Don't ask, Imgui init wants swapchain renderpass to be ready, but its not. So basically we need 2 init phases..
#ifdef RABBITHOLE_USING_IMGUI
	bool isInEditorMode = true;
#else
	bool isInEditorMode = false;
#endif
private:
	void CalculateMatrices(VulkanglTFModel::Node* node, Vertex* vertexBufferCpu, uint32_t* indexBufferCpu, std::vector<bool>& verticesMultipliedWithMatrix);
};
