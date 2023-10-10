#include "Renderer.h"

#include "Render/BVH.h"
#include "Render/Camera.h"
#include "Render/Converters.h"
#include "Render/PipelineManager.h"
#include "Render/RabbitPasses/Tools.h"
#include "Render/RabbitPasses/Postprocessing.h"
#include "Render/ResourceStateTracking.h"
#include "Render/Shader.h"
#include "Render/SuperResolutionManager.h"
#include "Render/Window.h"
#include "Utils/Utils.h"
#include "Vulkan/Include/VulkanWrapper.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <optick/src/optick.h>

#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <set>
#include <sstream>
#include <thread>
#include <vector>
#include <cstdio>

bool Renderer::Init()
{
	m_ResFolder = Utils::FindResFolder().string();
	m_TimeWhenRendererStarted = static_cast<float>(glfwGetTime());

	m_MainCamera.Init();
	SuperResolutionManager::instance().Init(&m_VulkanDevice);

	CreateSwapchain();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateCommandBuffers();
	LoadAndCreateShaders();

	// initialize Optick gpu
	auto graphicsQueue = m_VulkanDevice.GetGraphicsQueue();
	auto graphicsDevice = m_VulkanDevice.GetGraphicDevice();
	auto physicalDevice = m_VulkanDevice.GetPhysicalDevice();
	auto graphicsFamily = m_VulkanDevice.FindPhysicalQueueFamilies().graphicsFamily;
	OPTICK_GPU_INIT_VULKAN(&graphicsDevice, &physicalDevice, &graphicsQueue, &graphicsFamily, 1, nullptr);

	InitDefaultTextures();
	LoadModels();
	InitLights();
#if defined(VULKAN_HWRT)
	RayTracing::InitRTFunctions(m_VulkanDevice);
	//init acceleration structure
	CreateAccelerationStructure();
#else
	ConstructBVH();
#endif

	m_RabbitPassManager.SchedulePasses(*this);
	m_RabbitPassManager.DeclareResources();

	m_GPUTimeStamps.OnCreate(&m_VulkanDevice, m_VulkanSwapchain->GetImageCount());

	//for now max 10240 commands
	m_GeometryIndirectDrawBuffer = new IndexedIndirectBuffer(m_VulkanDevice, 10240);
	m_IndirectCloudDrawBuffer = new IndexedIndirectBuffer(m_VulkanDevice, 10240);

	return true;
}

bool Renderer::Shutdown()
{
	VULKAN_API_CALL(vkDeviceWaitIdle(m_VulkanDevice.GetGraphicDevice()));

#if defined(VULKAN_HWRT)
	m_VulkanDevice.pfnDestroyAccelerationStructureKHR(m_VulkanDevice.GetGraphicDevice(), TLAS.accelerationStructure, nullptr);
	m_VulkanDevice.pfnDestroyAccelerationStructureKHR(m_VulkanDevice.GetGraphicDevice(), BLAS.accelerationStructure, nullptr);
#endif

	delete(m_GeometryIndirectDrawBuffer);
	delete(m_IndirectCloudDrawBuffer);
	gltfModels.clear();
	m_GPUTimeStamps.OnDestroy();
	SuperResolutionManager::instance().Destroy();
	m_PipelineManager.Destroy();

    return true;
}

void Renderer::Clear() const
{
	m_GeometryIndirectDrawBuffer->Reset();
	m_IndirectCloudDrawBuffer->Reset();
}

void Renderer::Draw(float dt)
{
	m_CurrentDeltaTime = dt;

	m_MainCamera.Update(dt);

    DrawFrame();

	m_CurrentFrameIndex++;

	Clear();
}

void Renderer::DrawFrame()
{
	OPTICK_EVENT();

	auto result = m_VulkanSwapchain->AcquireNextImage(&m_CurrentImageIndex);
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
	{
		LOG_ERROR("failed to acquire swap chain image!");
	}

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		RecreateSwapchain();
		return;
	}

	std::string shaderChanged;
	if (m_ShaderCompiler.Update(shaderChanged))
	{
		// if shaderChanged is slang file
		auto foundLastDot = shaderChanged.find_last_of('.');
		auto fileExtension = shaderChanged.substr(foundLastDot + 1);
		if (fileExtension.compare("slang") == 0)
			CompileShader(shaderChanged);
	}

	auto startTime = Utils::ProfileSetStartTime();
	RecordCommandBuffer();
	m_CurrentCPUTimeInMS = Utils::ProfileGetEndTime(startTime) / 1000.f;

	result = m_VulkanSwapchain->SubmitCommandBufferAndPresent(GetCurrentCommandBuffer(), &m_CurrentImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
	{
		m_FramebufferResized = false;
		RecreateSwapchain();
	}
}

void Renderer::InitDefaultTextures()
{
	g_DefaultWhiteTexture = m_ResourceManager.CreateTexture(m_VulkanDevice, m_ResFolder + "\\textures\\default_white.png", ROTextureCreateInfo{
			.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst},
			.format = {Format::R8G8B8A8_UNORM_SRGB},
			.name = {"Default White"}
		});
	
	g_DefaultBlackTexture = m_ResourceManager.CreateTexture(m_VulkanDevice, m_ResFolder + "\\textures\\default_black.png", ROTextureCreateInfo{
			.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst},
			.format = {Format::R8G8B8A8_UNORM_SRGB},
			.name = {"Default Black"}
		});
	
	g_Default3DTexture = m_ResourceManager.CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {4, 4, 4},
			.flags = {TextureFlags::Color | TextureFlags::Read},
			.format = {Format::B8G8R8A8_UNORM},
			.name = {"Default 3D"}
		});
	
	g_DefaultArrayTexture = m_ResourceManager.CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {4, 4, 1},
			.flags = {TextureFlags::Color | TextureFlags::Read},
			.format = {Format::B8G8R8A8_UNORM},
			.name = {"Default Array"},
			.arraySize = {4}
		});

	noise2DTexture = m_ResourceManager.CreateTexture(m_VulkanDevice, m_ResFolder + "\\textures\\noise3.png", ROTextureCreateInfo{
			.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst},
			.format = {Format::B8G8R8A8_UNORM},
			.name = {"Noise2D"}
		});

	blueNoise2DTexture = m_ResourceManager.CreateTexture(m_VulkanDevice, m_ResFolder + "\\textures\\noise.png", ROTextureCreateInfo{
			.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst | TextureFlags::Storage},
			.format = {Format::B8G8R8A8_UNORM},
			.name = {"BlueNoise2D"}
		});

	noise3DLUT = m_ResourceManager.CreateTexture(m_VulkanDevice, RWTextureCreateInfo{
			.dimensions = {64, 64, 64},
			.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::Storage},
			.format = {Format::R8G8B8A8_UNORM},
			.name = {"Noise 3D LUT"}
		});
}

void Renderer::CalculateMatrices(VulkanglTFModel::Node* node, Vertex* vertexBufferCpu, uint32_t* indexBufferCpu, std::vector<bool>& verticesMultipliedWithMatrix)
{
	Matrix44f nodeMatrix = node->matrix;
	VulkanglTFModel::Node* currentParent = node->parent;

	while (currentParent)
	{
		nodeMatrix = currentParent->matrix * nodeMatrix;
		currentParent = currentParent->parent;
	}

	for (auto& primitive : node->mesh.primitives)
	{
		for (uint32_t i = primitive.firstIndex; i < primitive.firstIndex + primitive.indexCount; i++)
		{
			auto currentIndex = indexBufferCpu[i];
			if (!verticesMultipliedWithMatrix[currentIndex])
			{
				vertexBufferCpu[currentIndex].position = nodeMatrix * Vector4f{ vertexBufferCpu[currentIndex].position, 1 };
				verticesMultipliedWithMatrix[currentIndex] = true;
			}
		}
	}

	for (auto& child : node->children)
	{
		CalculateMatrices(&child, vertexBufferCpu, indexBufferCpu, verticesMultipliedWithMatrix);
	}
}

void Renderer::LoadModels()
{
	gltfModels.emplace_back(this, m_ResFolder + "\\meshes\\separateObjects.gltf", RenderingContext::GBuffer_Opaque);
	//gltfModels.emplace_back(this,  m_ResFolder + "\\meshes\\cottage.gltf", RenderingContext::GBuffer_Opaque);
	//gltfModels.emplace_back(this, m_ResFolder + "\\meshes\\sponza\\sponza.gltf", RenderingContext::GBuffer_Opaque);
	//gltfModels.emplace_back(this, m_ResFolder + "\\meshes\\sponzaNew\\MainSponza.gltf", RenderingContext::GBuffer_Opaque);

	//cloudMeshes.emplace_back(this, m_ResFolder + "\\meshes\\simpleShapes\\sphere.gltf", RenderingContext::Clouds_Transparent);
	//cloudMeshes.emplace_back(this, m_ResFolder + "\\meshes\\simpleShapes\\box.obj", RenderingContext::Clouds_Transparent);
}

void Renderer::BeginLabel(const char* name)
{
	m_VulkanDevice.BeginLabel(GetCurrentCommandBuffer(), name);
}

void Renderer::EndLabel()
{
	m_VulkanDevice.EndLabel(GetCurrentCommandBuffer());
}

void Renderer::RecordGPUTimeStamp(const char* label)
{
	if (m_RecordGPUTimeStamps)
		m_GPUTimeStamps.GetTimeStamp(GetCurrentCommandBuffer(), label);
}

void Renderer::DrawGeometryGLTF(std::vector<VulkanglTFModel>& bucket)
{
	m_ResourceStateTrackingManager.CommitBarriers(*this);

	//renderpass
	std::vector<VulkanImageView*>& attachments = m_StateManager.GetRenderTargets();
	VulkanImageView* depthStencil = m_StateManager.GetDepthStencil();
	RenderPassInfo* renderPassInfo = m_StateManager.GetRenderPassInfo();

	RenderPass* renderpass =
		m_StateManager.GetRenderPassDirty()
		? m_PipelineManager.FindOrCreateRenderPass(m_VulkanDevice, attachments, depthStencil, *renderPassInfo)
		: m_StateManager.GetRenderPass();

	m_StateManager.SetRenderPass(renderpass);

	//pipeline
	PipelineInfo* pipelineInfo = m_StateManager.GetPipelineInfo();

	pipelineInfo->renderPass = &m_StateManager.GetRenderPass()->GetVulkanRenderPass();
	VulkanPipeline* pipeline =
		m_StateManager.GetPipelineDirty()
		? m_PipelineManager.FindOrCreateGraphicsPipeline(m_VulkanDevice, *pipelineInfo)
		: m_StateManager.GetPipeline();

	m_StateManager.SetPipeline(pipeline);

	pipeline->Bind(GetCurrentCommandBuffer());

	m_StateManager.GetRenderPass()->BeginRenderPass(GetCurrentCommandBuffer());

	VulkanglTFModel::ms_CurrentDrawId = 0;

	for (auto& model : bucket)
	{
		model.BindBuffers(GetCurrentCommandBuffer());

		model.Draw(GetCurrentCommandBuffer(), m_StateManager.GetPipeline()->GetPipelineLayout(), m_CurrentImageIndex, m_GeometryIndirectDrawBuffer);
	}

	m_GeometryIndirectDrawBuffer->SubmitToGPU();

	m_StateManager.GetRenderPass()->EndRenderPass(GetCurrentCommandBuffer());

	m_StateManager.Reset();
}

void Renderer::DrawClouds(std::vector<VulkanglTFModel>& bucket)
{
	BindPipeline<GraphicsPipeline>();

	m_StateManager.GetRenderPass()->BeginRenderPass(GetCurrentCommandBuffer());

	for (auto& model : bucket)
	{
		model.BindBuffers(GetCurrentCommandBuffer());

		model.Draw(GetCurrentCommandBuffer(), m_StateManager.GetPipeline()->GetPipelineLayout(), m_CurrentImageIndex, m_IndirectCloudDrawBuffer);
	}

	m_IndirectCloudDrawBuffer->SubmitToGPU();

	m_StateManager.GetRenderPass()->EndRenderPass(GetCurrentCommandBuffer());

	m_StateManager.Reset();
}

void Renderer::DrawFullScreenQuad()
{
	BindPipeline<GraphicsPipeline>();

	m_StateManager.GetRenderPass()->BeginRenderPass(GetCurrentCommandBuffer());

	vkCmdDraw(GET_VK_HANDLE(GetCurrentCommandBuffer()), 3, 1, 0, 0);

	m_StateManager.GetRenderPass()->EndRenderPass(GetCurrentCommandBuffer());
	
	m_StateManager.Reset();
}


void Renderer::TraceRays(uint32_t x, uint32_t y, uint32_t z)
{
#if defined(VULKAN_HWRT)
	BindPipeline<RayTracingPipeline>();
	
	VkStridedDeviceAddressRegionKHR emptySbtEntry = {};
	auto& bindingTables = m_StateManager.GetPipeline()->GetBindingTables();
	m_VulkanDevice.pfnCmdTraceRaysKHR(
		GET_VK_HANDLE(GetCurrentCommandBuffer()),
		&bindingTables.raygen.stridedDeviceAddressRegion,
		&bindingTables.miss.stridedDeviceAddressRegion,
		&bindingTables.hit.stridedDeviceAddressRegion,
		&emptySbtEntry,
		x,
		y,
		z);
#endif
}

void Renderer::BindPushConstInternal()
{
	if (m_StateManager.ShouldBindPushConst())
	{
		vkCmdPushConstants(GET_VK_HANDLE(GetCurrentCommandBuffer()),
			GET_VK_HANDLE_PTR(m_StateManager.GetPipeline()->GetPipelineLayout()),
			GetVkShaderStageFrom(m_StateManager.GetPipeline()->GetType()),
			0,
			m_StateManager.GetPushConst()->size,
			m_StateManager.GetPushConst()->data);

		m_StateManager.SetShouldBindPushConst(false);
	}
}

void Renderer::CopyToSwapChain()
{
	BindPipeline<GraphicsPipeline>();

	if (!m_ImGuiManager.IsInitialized())
	{
		m_ImGuiManager.Init(*this);
		m_ImGuiManager.RegisterTextureForImGui(TonemappingPass::Output);
		m_ImGuiManager.RegisterTextureForImGui(TextureDebugPass::Output);
	}

	m_StateManager.GetRenderPass()->BeginRenderPass(GetCurrentCommandBuffer());

	//if editor is activated render into ImGui image, else render full screen into swapchain
	if (!isInEditorMode)
		vkCmdDraw(GET_VK_HANDLE(GetCurrentCommandBuffer()), 3, 1, 0, 0);

	if (IsImguiReady())
	{
		if (isInEditorMode)
		{
			ImGui::Begin("Viewport");
			ImGui::Image(m_ImGuiManager.GetImGuiTextureFrom(TonemappingPass::Output), GetScaledSizeWithAspectRatioKept(ImVec2(static_cast<float>(GetUpscaledWidth), static_cast<float>(GetUpscaledHeight))));
			ImGui::End();
		}

		m_ImGuiManager.Render(*this);
	}

	m_StateManager.GetRenderPass()->EndRenderPass(GetCurrentCommandBuffer());
	
	m_StateManager.Reset();
}

void Renderer::BindCameraMatrices(Camera* camera)
{	
	auto projection = camera->Projection();
	auto view = camera->View();

	m_StateManager.UpdateUBOElement(UBOElement::PrevViewProjMatrix, 4, &m_CurrentCameraState.PrevViewProjMatrix);

	m_CurrentCameraState.ViewMatrix = view;
	m_StateManager.UpdateUBOElement(UBOElement::ViewMatrix, 4, &m_CurrentCameraState.ViewMatrix);

	m_CurrentCameraState.ProjectionMatrix = projection;
	m_StateManager.UpdateUBOElement(UBOElement::ProjectionMatrix, 4, &m_CurrentCameraState.ProjectionMatrix);

	//todo: double check this, for now I use jittered matrix in VS_Gbuffer and VS_Skybox
	//disable jitter when camera is moving
	if (m_CurrentCameraState.HasViewProjMatrixChanged)
		m_CurrentCameraState.ProjMatrixJittered = camera->Projection();
	else
		m_CurrentCameraState.ProjMatrixJittered = camera->ProjectionJittered();

	m_StateManager.UpdateUBOElement(UBOElement::ProjectionMatrixJittered, 4, &m_CurrentCameraState.ProjMatrixJittered);

	m_CurrentCameraState.ViewProjMatrix = projection * view;
	m_StateManager.UpdateUBOElement(UBOElement::ViewProjMatrix, 4, &m_CurrentCameraState.ViewProjMatrix);

	m_CurrentCameraState.CameraPosition = camera->GetPosition();
	m_StateManager.UpdateUBOElement(UBOElement::CameraPosition, 1, &m_CurrentCameraState.CameraPosition);
	
	m_CurrentCameraState.ViewInverseMatrix = glm::affineInverse(view);
	m_StateManager.UpdateUBOElement(UBOElement::ViewInverse, 4, &m_CurrentCameraState.ViewInverseMatrix);

	m_CurrentCameraState.ProjectionInverseMatrix = glm::inverse(projection);
	m_StateManager.UpdateUBOElement(UBOElement::ProjInverse, 4, &m_CurrentCameraState.ProjectionInverseMatrix);

	m_CurrentCameraState.ViewProjInverseMatrix = m_CurrentCameraState.ViewInverseMatrix * m_CurrentCameraState.ProjectionInverseMatrix;
	m_StateManager.UpdateUBOElement(UBOElement::ViewProjInverse, 4, &m_CurrentCameraState.ViewProjInverseMatrix);

	float width = projection[0][0];
	float height = projection[1][1];

	m_CurrentCameraState.EyeXAxis = m_CurrentCameraState.ViewInverseMatrix * Vector4f(-1.0 / width, 0, 0, 0);
	m_CurrentCameraState.EyeYAxis = m_CurrentCameraState.ViewInverseMatrix * Vector4f(0, -1.0 / height, 0, 0);
	m_CurrentCameraState.EyeZAxis = m_CurrentCameraState.ViewInverseMatrix * Vector4f(0, 0, 1.f, 0);

	m_StateManager.UpdateUBOElement(UBOElement::EyeXAxis, 1, &m_CurrentCameraState.EyeXAxis);
	m_StateManager.UpdateUBOElement(UBOElement::EyeYAxis, 1, &m_CurrentCameraState.EyeYAxis);
	m_StateManager.UpdateUBOElement(UBOElement::EyeZAxis, 1, &m_CurrentCameraState.EyeZAxis);

	if (m_CurrentCameraState.ViewProjMatrix == m_CurrentCameraState.PrevViewProjMatrix)
		m_CurrentCameraState.HasViewProjMatrixChanged = false;
	else
		m_CurrentCameraState.HasViewProjMatrixChanged = true;

	m_CurrentCameraState.PrevViewProjMatrix = m_CurrentCameraState.ViewProjMatrix;
	m_CurrentCameraState.PrevViewMatrix = m_CurrentCameraState.ViewMatrix;
}

void Renderer::BindUBO()
{	
	if (m_StateManager.GetUBODirty())
	{ 
		m_MainConstBuffer[m_CurrentImageIndex]->FillBuffer(m_StateManager.GetUBO(), sizeof(UniformBufferObject));
		
		m_StateManager.SetUBODirty(false);
	}
}

void Renderer::CreateAccelerationStructure()
{
#if defined(VULKAN_HWRT)
	std::vector<Vector4f> verticesFinal;

	uint32_t vertexOffset = 0;

	for (auto& model : gltfModels)
	{
		std::vector<bool> verticesMultipliedWithMatrix;

		auto modelVertexBuffer = model.GetVertexBuffer();
		auto modelIndexBuffer = model.GetIndexBuffer();

		//TODO: replace this with one Main Init Render command buffer with a lifetime only in init stages
		VulkanCommandBuffer tempCommandBuffer(m_VulkanDevice, "Temp command buffer");
		tempCommandBuffer.BeginCommandBuffer();

		VulkanBuffer stagingBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = BufferUsageFlags::TransferDst,
			.memoryAccess = MemoryAccess::CPU,
			.size = modelVertexBuffer->GetSize(),
			.name = "StagingBuffer"
		});
		m_VulkanDevice.CopyBuffer(tempCommandBuffer, *modelVertexBuffer, stagingBuffer, modelVertexBuffer->GetSize());

		Vertex* vertexBufferCpu = (Vertex*)stagingBuffer.Map();
		uint32_t vertexCount = static_cast<uint32_t>(modelVertexBuffer->GetSize() / sizeof(Vertex));
		verticesMultipliedWithMatrix.resize(vertexCount);

		VulkanBuffer stagingBuffer2(m_VulkanDevice, BufferCreateInfo{
			.flags = BufferUsageFlags::TransferDst,
			.memoryAccess = MemoryAccess::CPU,
			.size = modelIndexBuffer->GetSize(),
			.name = "StagingBuffer"
		}); 
		
		m_VulkanDevice.CopyBuffer(tempCommandBuffer, *modelIndexBuffer, stagingBuffer2, modelIndexBuffer->GetSize());

		tempCommandBuffer.EndAndSubmitCommandBuffer();

		uint32_t* indexBufferCpu = (uint32_t*)stagingBuffer2.Map();
		uint32_t indexCount = static_cast<uint32_t>(modelIndexBuffer->GetSize() / sizeof(uint32_t));

		for (auto& node : model.GetNodes())
		{
			CalculateMatrices(&node, vertexBufferCpu, indexBufferCpu, verticesMultipliedWithMatrix);
		}

		for (uint32_t k = 0; k < vertexCount; k++)
		{
			Vector4f position = Vector4f{ vertexBufferCpu[k].position, 1.f };
			verticesFinal.push_back(position);
		}

		vertexOffset += vertexCount;
	}
	//BLAS
	{
		VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
		VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};

		VulkanBuffer* sceneVertexBuffer = m_ResourceManager.CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::StorageBuffer | BufferUsageFlags::ShaderDeviceAddress | BufferUsageFlags::AccelerationStructureBuild },
			.memoryAccess = {MemoryAccess::GPU},
			.size = {static_cast<uint32_t>(verticesFinal.size() * sizeof(Vector4f))},
			.name = {"VertexBuffer"}
			});
		sceneVertexBuffer->FillBuffer(verticesFinal.data(), static_cast<uint32_t>(verticesFinal.size()) * sizeof(Vector4f));

		VulkanBuffer* sceneIndexBuffer = gltfModels[0].GetIndexBuffer();

		vertexBufferDeviceAddress.deviceAddress = RayTracing::GetBufferDeviceAddress(m_VulkanDevice, sceneVertexBuffer);
		indexBufferDeviceAddress.deviceAddress = RayTracing::GetBufferDeviceAddress(m_VulkanDevice, sceneIndexBuffer);

		uint32_t numTriangles = static_cast<uint32_t>(sceneIndexBuffer->GetSize() / sizeof(uint32_t)) / 3;
		m_TriangleCount = numTriangles;

		uint32_t maxVertex = static_cast<uint32_t>(sceneVertexBuffer->GetSize() / sizeof(Vector4f));

		VkAccelerationStructureGeometryKHR accelerationStructureGeometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
		accelerationStructureGeometry.geometry.triangles.maxVertex = maxVertex;
		accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(Vector4f);
		accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
		accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
		accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
		accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;

		// Get size info
		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
		accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationStructureBuildGeometryInfo.geometryCount = 1;
		accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
		m_VulkanDevice.pfnGetAccelerationStructureBuildSizesKHR(
			m_VulkanDevice.GetGraphicDevice(),
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&accelerationStructureBuildGeometryInfo,
			&numTriangles,
			&accelerationStructureBuildSizesInfo);

		RayTracing::CreateAccelerationStructure(this, BLAS, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, accelerationStructureBuildSizesInfo);

		RayTracing::ScratchBuffer scratchBuffer{};
		scratchBuffer.buffer = m_ResourceManager.CreateBuffer(m_VulkanDevice, BufferCreateInfo{
				.flags = {BufferUsageFlags::StorageBuffer | BufferUsageFlags::ShaderDeviceAddress},
				.memoryAccess = {MemoryAccess::GPU},
				.size = {static_cast<uint32_t>(accelerationStructureBuildSizesInfo.buildScratchSize)},
				.name = {"BLAS ScratchBuffer"}
			});
		scratchBuffer.deviceAddress = RayTracing::AlignedSize(RayTracing::GetBufferDeviceAddress(m_VulkanDevice, scratchBuffer.buffer), m_VulkanDevice.GetAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment);

		VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
		accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		accelerationBuildGeometryInfo.dstAccelerationStructure = BLAS.accelerationStructure;
		accelerationBuildGeometryInfo.geometryCount = 1;
		accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
		accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

		VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
		accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
		accelerationStructureBuildRangeInfo.primitiveOffset = 0;
		accelerationStructureBuildRangeInfo.firstVertex = 0;
		accelerationStructureBuildRangeInfo.transformOffset = 0;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

		VulkanCommandBuffer commandBuffer{ m_VulkanDevice, "BuildBLAS" };
		commandBuffer.BeginCommandBuffer(true);

		m_VulkanDevice.pfnCmdBuildAccelerationStructuresKHR(
			GET_VK_HANDLE(commandBuffer),
			1,
			&accelerationBuildGeometryInfo,
			accelerationBuildStructureRangeInfos.data());

		commandBuffer.EndAndSubmitCommandBuffer();

		m_ResourceManager.DeleteBuffer(scratchBuffer.buffer);
	}
	//TLAS
	{
		VkTransformMatrixKHR transformMatrix = {
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f };

		VkAccelerationStructureInstanceKHR instance{};
		instance.transform = transformMatrix;
		instance.instanceCustomIndex = 0;
		instance.mask = 0xFF;
		instance.instanceShaderBindingTableRecordOffset = 0;
		instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		instance.accelerationStructureReference = BLAS.deviceAddress;

		// Buffer for instance data
		VulkanBuffer* instancesBuffer = m_ResourceManager.CreateBuffer(m_VulkanDevice, BufferCreateInfo{
				.flags = { BufferUsageFlags::ShaderDeviceAddress | BufferUsageFlags::AccelerationStructureBuild },
				.memoryAccess = { MemoryAccess::CPU },
				.size = { sizeof(VkAccelerationStructureInstanceKHR) },
				.name = { "Instances Buffer" }
			});
		instancesBuffer->FillBuffer(&instance);

		VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
		instanceDataDeviceAddress.deviceAddress = RayTracing::GetBufferDeviceAddress(m_VulkanDevice, instancesBuffer);

		VkAccelerationStructureGeometryKHR accelerationStructureGeometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
		accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

		// Get size info
		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
		accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationStructureBuildGeometryInfo.geometryCount = 1;
		accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

		uint32_t primitive_count = 1;

		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
		m_VulkanDevice.pfnGetAccelerationStructureBuildSizesKHR(
			m_VulkanDevice.GetGraphicDevice(),
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&accelerationStructureBuildGeometryInfo,
			&primitive_count,
			&accelerationStructureBuildSizesInfo);

		// @todo: as return value?
		RayTracing::CreateAccelerationStructure(this, TLAS, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, accelerationStructureBuildSizesInfo);

		// Create a small scratch buffer used during build of the top level acceleration structure
		RayTracing::ScratchBuffer scratchBufferTLAS{};
		scratchBufferTLAS.buffer = m_ResourceManager.CreateBuffer(m_VulkanDevice, BufferCreateInfo{
				.flags = {BufferUsageFlags::StorageBuffer | BufferUsageFlags::ShaderDeviceAddress},
				.memoryAccess = {MemoryAccess::GPU},
				.size = {static_cast<uint32_t>(accelerationStructureBuildSizesInfo.buildScratchSize)},
				.name = {"TLAS ScratchBuffer"}
			});
		scratchBufferTLAS.deviceAddress = RayTracing::AlignedSize(RayTracing::GetBufferDeviceAddress(m_VulkanDevice, scratchBufferTLAS.buffer), m_VulkanDevice.GetAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment);

		VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
		accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		accelerationBuildGeometryInfo.dstAccelerationStructure = TLAS.accelerationStructure;
		accelerationBuildGeometryInfo.geometryCount = 1;
		accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
		accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBufferTLAS.deviceAddress;

		VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
		accelerationStructureBuildRangeInfo.primitiveCount = 1;
		accelerationStructureBuildRangeInfo.primitiveOffset = 0;
		accelerationStructureBuildRangeInfo.firstVertex = 0;
		accelerationStructureBuildRangeInfo.transformOffset = 0;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

		// Build the acceleration structure on the device via a one-time command buffer submission
		// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
		
		VulkanCommandBuffer commandBuffer{ m_VulkanDevice, "BuildTLAS" };
		commandBuffer.BeginCommandBuffer(true);

		m_VulkanDevice.pfnCmdBuildAccelerationStructuresKHR(
			GET_VK_HANDLE(commandBuffer),
			1,
			&accelerationBuildGeometryInfo,
			accelerationBuildStructureRangeInfos.data());

		commandBuffer.EndAndSubmitCommandBuffer();

		m_ResourceManager.DeleteBuffer(scratchBufferTLAS.buffer);
		m_ResourceManager.DeleteBuffer(instancesBuffer);
	}
#endif
}

void Renderer::BindDescriptorSets()
{
	VulkanDescriptorSet* descriptorSet = m_StateManager.FinalizeDescriptorSet(m_VulkanDevice, m_DescriptorPool.get());
	VulkanPipeline* pipeline = m_StateManager.GetPipeline();

	vkCmdBindDescriptorSets(
		GET_VK_HANDLE(GetCurrentCommandBuffer()), 
		GetVkBindPointFrom(pipeline->GetType()),
		GET_VK_HANDLE_PTR(pipeline->GetPipelineLayout()),
		0, 
		1, GET_VK_HANDLE_PTR(descriptorSet), 
		0, 
		nullptr);
}

void Renderer::CompileShader(const std::string& name, const std::string& entryPoint, std::vector<const char*>defines)
{
	void* data = nullptr;
	size_t size;
	
	bool success = m_ShaderCompiler.CompileShader(name, entryPoint, &data, &size, defines);
	if (!success) return;

	std::string fileNameFinal = name.substr(0, name.find_last_of('.'));
	
	ShaderInfo createInfo{};
	createInfo.CodeEntry = entryPoint.c_str();
	createInfo.Type = GetShaderStageFrom(name);

	if (entryPoint.compare("main"))
	{
		fileNameFinal += "_" + entryPoint;
	}

	m_ResourceManager.CreateShader(m_VulkanDevice, createInfo, reinterpret_cast<const char*>(data), size, fileNameFinal.c_str());
	
	free(data);
}

void Renderer::LoadAndCreateShaders()
{
	//TODO: implement real shader compiler and stuff
	std::filesystem::path currentPath = Utils::FindResFolder();
	currentPath += "\\shaders";

	for (const auto& file : std::filesystem::directory_iterator(currentPath))
	{
		std::string filePath = file.path().string();
		auto foundLastSlash = filePath.find_last_of("/\\");

		if (foundLastSlash)
		{
			std::string fileNameWithExt = filePath.substr(foundLastSlash + 1);
			auto foundLastDot = fileNameWithExt.find_last_of(".");

			if (foundLastDot)
			{
				std::string fileExtension = fileNameWithExt.substr(foundLastDot + 1);

				//for spv files create shader modules
				if (fileExtension.compare("spv") == 0)
				{
					std::string fileNameFinal = fileNameWithExt.substr(0, foundLastDot);

					auto shaderCode = Utils::ReadFile(filePath);

					ShaderInfo createInfo{};
					createInfo.CodeEntry = nullptr;
					createInfo.Type = GetShaderStageFrom(fileNameFinal);

					m_ResourceManager.CreateShader(m_VulkanDevice, createInfo, shaderCode.data(), shaderCode.size(), fileNameFinal.c_str());
				}
			}
		}
	}

	CompileShader("CS_FilterSoftShadows.slang", "Pass0");
	CompileShader("CS_FilterSoftShadows.slang", "Pass1");
	CompileShader("CS_FilterSoftShadows.slang", "Pass2");
	CompileShader("CS_PrepareShadowMask.slang");
	CompileShader("CS_TileClassification.slang");
	CompileShader("FS_VolumetricClouds.slang");
	CompileShader("VS_PassThrough.slang");
	CompileShader("FS_PassThrough.slang");
	CompileShader("VS_GBuffer.slang");
	CompileShader("VS_Skybox.slang");
	CompileShader("FS_PBR.slang");
	CompileShader("FS_CopyDepth.slang");
	CompileShader("FS_GBuffer.slang");
	CompileShader("FS_Skybox.slang");
	CompileShader("CS_SSAO.slang");
	CompileShader("FS_SSAOBlur.slang");
	CompileShader("CS_RayTracingShadows.slang", "main", {"SOFT_SHADOWS"});
	CompileShader("CS_Volumetric.slang", "CalculateDensity");
	CompileShader("CS_Volumetric.slang", "CalculateVolumetricShadows");
	CompileShader("CS_3DNoiseLUT.slang");
	CompileShader("CS_ComputeScattering.slang");
	CompileShader("FS_ApplyVolumetricFog.slang");
	CompileShader("FS_Tonemap.slang");
	CompileShader("FS_ApplyBloom.slang");
	CompileShader("FS_TextureDebug.slang");
	CompileShader("CS_Downsample.slang");
	CompileShader("CS_Upsample.slang");
	CompileShader("FS_SSAOBlur.slang");
}

void Renderer::CreateCommandBuffers() 
{
	m_MainRenderCommandBuffers.resize(m_VulkanSwapchain->GetImageCount());

	for (uint32_t i = 0; i < m_VulkanSwapchain->GetImageCount(); i++)
	{
		m_MainRenderCommandBuffers[i] = std::make_unique<VulkanCommandBuffer>(m_VulkanDevice, "Main Render Command Buffer");
	}
}

void Renderer::CreateSwapchain()
{
	auto nativeWindowHandle = Window::instance().GetNativeWindowHandle();

	int currentWidth = Window::instance().GetExtent().width;
	int currentHeight = Window::instance().GetExtent().height;

	/* handles minimization of a window ? */
	while (currentWidth == 0 || currentHeight == 0)
	{
		glfwGetFramebufferSize(nativeWindowHandle, &currentWidth, &currentHeight);
		glfwWaitEvents();
	}

	VULKAN_API_CALL(vkDeviceWaitIdle(m_VulkanDevice.GetGraphicDevice()));

	m_VulkanSwapchain = std::make_unique<VulkanSwapchain>(m_VulkanDevice, Extent2D{ static_cast<uint32_t>(currentWidth), static_cast<uint32_t>(currentHeight) });
}

void Renderer::RecreateSwapchain()
{
	auto nativeWindowHandle = Window::instance().GetNativeWindowHandle();

	int currentWidth = Window::instance().GetExtent().width;
	int currentHeight = Window::instance().GetExtent().height;

	/* handles minimization of a window ? */
	while (currentWidth == 0 || currentHeight == 0)
	{
		glfwGetFramebufferSize(nativeWindowHandle, &currentWidth, &currentHeight);
		glfwWaitEvents();
	}

	VULKAN_API_CALL(vkDeviceWaitIdle(m_VulkanDevice.GetGraphicDevice()));

	m_VulkanSwapchain = std::make_unique<VulkanSwapchain>(m_VulkanDevice, Extent2D{ static_cast<uint32_t>(currentWidth), static_cast<uint32_t>(currentHeight) }, GET_VK_HANDLE_PTR(m_VulkanSwapchain.get()));
}

void Renderer::ResourceBarrier(VulkanTexture* texture, ResourceState oldLayout, ResourceState newLayout, ResourceStage srcStage, ResourceStage dstStage, uint32_t mipLevel, uint32_t mipCount)
{
	m_VulkanDevice.ResourceBarrier(GetCurrentCommandBuffer(), texture, oldLayout, newLayout, srcStage, dstStage, mipLevel, mipCount);
}

void Renderer::ResourceBarrier(VulkanBuffer* buffer, ResourceState oldLayout, ResourceState newLayout, ResourceStage srcStage, ResourceStage dstStage)
{
	m_VulkanDevice.ResourceBarrier(GetCurrentCommandBuffer(), buffer, oldLayout, newLayout, srcStage, dstStage);
}

void Renderer::RecordCommandBuffer()
{
	OPTICK_EVENT();

	GetCurrentCommandBuffer().BeginCommandBuffer();

	OPTICK_GPU_CONTEXT(GET_VK_HANDLE(GetCurrentCommandBuffer()));

	std::vector<TimeStamp> timeStamps{};
	if (m_RecordGPUTimeStamps)
	{
		m_GPUTimeStamps.OnBeginFrame(GetCurrentCommandBuffer(), &timeStamps);
		m_GPUTimeStamps.GetTimeStamp(GetCurrentCommandBuffer(), "Begin of the frame");
	}

	if (m_ImGuiManager.IsInitialized())
	{
		//make imgui window pos and size same as the main window
		m_ImGuiManager.NewFrame(0.f, 0.f, static_cast<float>(Window::instance().GetExtent().width), static_cast<float>(Window::instance().GetExtent().height));

		ImGui::Begin("Main debug frame");
		ImGui::Checkbox("GPU Profiler Enabled: ", &m_RecordGPUTimeStamps);
		ImGui::End();

		ImGuiTextureDebugger();

		if (m_RecordGPUTimeStamps)
		{
			ImguiProfilerWindow(timeStamps);
		}

		m_ImGuiManager.MakeReady();
	}

	UpdateUIStateAndFSR2PreDraw();
	UpdateConstantBuffer();
	BindCameraMatrices(&m_MainCamera);
	BindUBO();

	EXECUTE_ONCE(m_RabbitPassManager.ExecuteOneTimePasses(*this));
	m_RabbitPassManager.ExecutePasses(*this);

	if (m_RecordGPUTimeStamps)
	{
		m_GPUTimeStamps.OnEndFrame();
	}

	GetCurrentCommandBuffer().EndCommandBuffer();
}


void Renderer::CreateUniformBuffers()
{
	for (uint32_t i = 0; i < m_VulkanSwapchain->GetImageCount(); i++)
	{
		m_MainConstBuffer[i] = m_ResourceManager.CreateBuffer(m_VulkanDevice, BufferCreateInfo{
				.flags = {BufferUsageFlags::UniformBuffer},
				.memoryAccess = {MemoryAccess::CPU2GPU},
				.size = {sizeof(UniformBufferObject)},
				.name = {"MainConstantBuffer"}
			});
	}

	m_VertexUploadBuffer = m_ResourceManager.CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::VertexBuffer},
			.memoryAccess = {MemoryAccess::CPU2GPU},
			.size = {MB_16},
			.name = {"Vertex Upload"}
		});
}

void Renderer::CreateDescriptorPool()
{
	//TODO: see what to do with this, now its hard coded number of descriptors
	VulkanDescriptorPoolSize uboPoolSize{};
	uboPoolSize.Count = 400;
	uboPoolSize.Type = DescriptorType::UniformBuffer;

	VulkanDescriptorPoolSize asPoolSize{};
	asPoolSize.Count = 20;
	asPoolSize.Type = DescriptorType::AccelerationStructure;

	VulkanDescriptorPoolSize samImgPoolSize{};
	samImgPoolSize.Count = 400;
	samImgPoolSize.Type = DescriptorType::SampledImage;

	VulkanDescriptorPoolSize sPoolSize{};
	sPoolSize.Count = 400;
	sPoolSize.Type = DescriptorType::Sampler;

	VulkanDescriptorPoolSize cisPoolSize{};
	cisPoolSize.Count = 400;
	cisPoolSize.Type = DescriptorType::CombinedSampler;

	VulkanDescriptorPoolSize siPoolSize{};
	siPoolSize.Count = 400;
	siPoolSize.Type = DescriptorType::StorageImage;

	VulkanDescriptorPoolSize sbPoolSize{};
	sbPoolSize.Count = 400;
	sbPoolSize.Type = DescriptorType::StorageBuffer;

	VulkanDescriptorPoolInfo vulkanDescriptorPoolInfo{};

	vulkanDescriptorPoolInfo.DescriptorSizes = { uboPoolSize, asPoolSize, cisPoolSize, siPoolSize, sbPoolSize, samImgPoolSize, sPoolSize };

	vulkanDescriptorPoolInfo.MaxSets = 2000;

	m_DescriptorPool = std::make_unique<VulkanDescriptorPool>(&m_VulkanDevice, vulkanDescriptorPoolInfo);
}

void Renderer::InitLights()
{
	lights.push_back(
		LightParams{
			.position = { 70.0f, 200.0f, -100.0f },
			.radius = {1.f},
			.color = {1.f, 0.6f, 0.2f},
			.intensity = {3.f},
			.type = {LightType::LightType_Directional},
			.size = {1.f}
		});

	lights.push_back(
		LightParams{
			.position = { -10.f, 1.2f, -3.0f },
			.radius = {12.f},
			.color = {1.f, 0.1f, 0.1f},
			.intensity = {1.f},
			.type = {LightType::LightType_Point},
			.size = {1.f}
		});

	lights.push_back(
		LightParams{
			.position = {-9.6f, 0.4f, 2.3f},
			.radius = {12.f},
			.color = { 0.f, 0.732f, 0.36f },
			.intensity = {0.6f},
			.type = {LightType::LightType_Point},
			.size = {0.2f}
		});

	lights.push_back(
		LightParams{
			.position = {-8.6f, 5.4f, 3.6f},
			.radius = {0.f},
			.color = {0.f, 0.3f, 0.9f},
			.intensity = {1.7f},
			.type = {LightType::LightType_Point},
			.size = {1.f},
		});
}

void Renderer::ConstructBVH()
{
	std::vector<BVH::Triangle> triangles;
	std::vector<Vector4f> verticesFinal;

	uint32_t vertexOffset = 0;

	for (auto& model : gltfModels)
	{
		std::vector<bool> verticesMultipliedWithMatrix;

		auto modelVertexBuffer = model.GetVertexBuffer();
		auto modelIndexBuffer = model.GetIndexBuffer();

		//TODO: replace this with one Main Init Render command buffer with a lifetime only in init stages
		VulkanCommandBuffer tempCommandBuffer(m_VulkanDevice, "Temp command buffer");
		tempCommandBuffer.BeginCommandBuffer();

		VulkanBuffer stagingBuffer(m_VulkanDevice, BufferCreateInfo{ 
			.flags = BufferUsageFlags::TransferDst,
			.memoryAccess = MemoryAccess::CPU, 
			.size = modelVertexBuffer->GetSize(), 
			.name = "StagingBuffer" 
		});
		m_VulkanDevice.CopyBuffer(tempCommandBuffer, *modelVertexBuffer, stagingBuffer, modelVertexBuffer->GetSize());

		Vertex* vertexBufferCpu = (Vertex*)stagingBuffer.Map();
		uint32_t vertexCount = static_cast<uint32_t>(modelVertexBuffer->GetSize() / sizeof(Vertex));
		verticesMultipliedWithMatrix.resize(vertexCount);

		VulkanBuffer stagingBuffer2(m_VulkanDevice, BufferCreateInfo{
			.flags = BufferUsageFlags::TransferDst,
			.memoryAccess = MemoryAccess::CPU,
			.size = modelIndexBuffer->GetSize(),
			.name = "StagingBuffer"
		});
		m_VulkanDevice.CopyBuffer(tempCommandBuffer, *modelIndexBuffer, stagingBuffer2, modelIndexBuffer->GetSize());

		tempCommandBuffer.EndAndSubmitCommandBuffer();

		uint32_t* indexBufferCpu = (uint32_t*)stagingBuffer2.Map();
		uint32_t indexCount = static_cast<uint32_t>(modelIndexBuffer->GetSize() / sizeof(uint32_t));
		
		for (auto& node : model.GetNodes())
		{
			CalculateMatrices(&node, vertexBufferCpu, indexBufferCpu, verticesMultipliedWithMatrix);
		}

		for (uint32_t k = 0; k < vertexCount; k++)
		{
			Vector4f position = Vector4f{ vertexBufferCpu[k].position, 1.f };
			verticesFinal.push_back(position);
		}

		for (uint32_t j = 0; j < indexCount; j += 3)
		{
			BVH::Triangle tri;
			tri.indices[0] = vertexOffset + indexBufferCpu[j];
			tri.indices[1] = vertexOffset + indexBufferCpu[j + 1];
			tri.indices[2] = vertexOffset + indexBufferCpu[j + 2];

			triangles.push_back(tri);
		}

		vertexOffset += vertexCount;
	}

	std::cout << triangles.size() << " triangles!" << std::endl;

	uint32_t* triIndices;
	uint32_t indicesNum = 0;

	BVH::CacheFriendlyBVHNode* root;
	uint32_t nodeNum = 0;

	bool createBVH = false;
	if (createBVH)
	{
		//create and store BVH data in file
		auto node = CreateBVH(verticesFinal, triangles);
		CreateCFBVH(triangles.data(), node, &triIndices, &indicesNum, &root, &nodeNum);

		FILE* dat;
		fopen_s(&dat, (m_ResFolder + "\\bvhdata\\separateObjects.bin").c_str(), "wb");

		std::fwrite(&indicesNum, sizeof(uint32_t), 1, dat);
		std::fwrite(triIndices, sizeof(uint32_t) * indicesNum, 1, dat);
		std::fwrite(&nodeNum, sizeof(uint32_t), 1, dat);
		std::fwrite(root, sizeof(BVH::CacheFriendlyBVHNode), nodeNum, dat);

		std::fclose(dat);
	}
	else
	{
		//load BVH data from file
		FILE* dat;
		fopen_s(&dat, (m_ResFolder + "\\bvhdata\\sponza.bin").c_str(), "rb");

		std::fread(&indicesNum, sizeof uint32_t, 1, dat);
		triIndices = RABBIT_ALLOC(uint32_t, indicesNum);
		std::fread(triIndices, sizeof uint32_t * indicesNum, 1, dat);
		std::fread(&nodeNum, sizeof uint32_t, 1, dat);
		root = RABBIT_ALLOC(BVH::CacheFriendlyBVHNode, nodeNum);
		std::fread(root, sizeof(BVH::CacheFriendlyBVHNode), nodeNum, dat);

		std::fclose(dat);
	}

	vertexBuffer = m_ResourceManager.CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::StorageBuffer},
			.memoryAccess = {MemoryAccess::GPU},
			.size = {static_cast<uint32_t>(verticesFinal.size() * sizeof(Vector4f))},
			.name = {"VertexBuffer"}
		});

	trianglesBuffer = m_ResourceManager.CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::StorageBuffer},
			.memoryAccess = {MemoryAccess::GPU},
			.size = {static_cast<uint32_t>(triangles.size() * sizeof(BVH::Triangle))},
			.name = {"TrianglesBuffer"}
		});

	triangleIndxsBuffer = m_ResourceManager.CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::StorageBuffer},
			.memoryAccess = {MemoryAccess::GPU},
			.size = {static_cast<uint32_t>(indicesNum * sizeof(uint32_t))},
			.name = {"TrianglesIndexBuffer"}
		});

	cfbvhNodesBuffer = m_ResourceManager.CreateBuffer(m_VulkanDevice, BufferCreateInfo{
			.flags = {BufferUsageFlags::StorageBuffer},
			.memoryAccess = {MemoryAccess::GPU},
			.size = {static_cast<uint32_t>(nodeNum * sizeof(BVH::CacheFriendlyBVHNode))},
			.name = {"CfbvhNodes"}
		});
   
	vertexBuffer->FillBuffer(verticesFinal.data(), static_cast<uint32_t>(verticesFinal.size()) * sizeof(Vector4f));
	trianglesBuffer->FillBuffer(triangles.data(), static_cast<uint32_t>(triangles.size()) * sizeof(BVH::Triangle));
	triangleIndxsBuffer->FillBuffer(triIndices, indicesNum * sizeof(uint32_t));
	cfbvhNodesBuffer->FillBuffer(root, nodeNum * sizeof(BVH::CacheFriendlyBVHNode));

	m_TriangleCount = static_cast<uint32_t>(trianglesBuffer->GetSize() / sizeof(BVH::Triangle));

	RABBIT_FREE(triIndices);
	RABBIT_FREE(root);
}

void Renderer::UpdateConstantBuffer()
{
	Vector4f frustrumInfo{};
	frustrumInfo.x = static_cast<float>(GetNativeWidth);
	frustrumInfo.y = static_cast<float>(GetNativeHeight);
    frustrumInfo.z = m_MainCamera.GetNearPlane();
    frustrumInfo.w = m_MainCamera.GetFarPlane();

    m_StateManager.UpdateUBOElement(UBOElement::FrustrumInfo, 1, &frustrumInfo);

	Vector4f frameInfo{};
	frameInfo.x = static_cast<float>(m_CurrentFrameIndex);
	frameInfo.y = static_cast<float>(glfwGetTime()) - m_TimeWhenRendererStarted;
	frameInfo.z = m_CurrentDeltaTime;

	m_StateManager.UpdateUBOElement(UBOElement::CurrentFrameInfo, 1, &frameInfo);
}

void Renderer::UpdateUIStateAndFSR2PreDraw()
{
	m_CurrentUIState.camera = &m_MainCamera;
	m_CurrentUIState.deltaTime = m_CurrentDeltaTime * 1000.f; //needs to be in ms
	m_CurrentUIState.renderHeight = static_cast<float>(GetNativeHeight);
	m_CurrentUIState.renderWidth = static_cast<float>(GetNativeWidth);
	m_CurrentUIState.sharpness = 0.5f;
	m_CurrentUIState.reset = m_CurrentCameraState.HasViewProjMatrixChanged;
	m_CurrentUIState.useRcas = true;
	m_CurrentUIState.useTaa = true;

	SuperResolutionManager::instance().PreDraw(&m_CurrentUIState);
}

void Renderer::ImguiProfilerWindow(std::vector<TimeStamp>& timeStamps)
{
	ImGui::Begin("GPU Profiler");

	ImGui::TextWrapped("GPU Adapter: %s", m_VulkanDevice.GetPhysicalDeviceProperties().properties.deviceName);
	ImGui::Text("Display Resolution : %ix%i", GetUpscaledWidth, GetUpscaledHeight);
	ImGui::Text("Render Resolution : %ix%i", GetNativeWidth, GetNativeHeight);

	uint32_t fps = static_cast<uint32_t>(1.f / m_CurrentDeltaTime);
	float frameTime_ms = m_CurrentDeltaTime * 1000.f;
	float numOfTriangles = m_TriangleCount / 1000.f;

	ImGui::Text("FPS        : %d (%.2f ms)", fps, frameTime_ms);
	ImGui::Text("CPU        :     (%.2f ms)", m_CurrentCPUTimeInMS);
	ImGui::Text("Num of triangles   : %.2fk", numOfTriangles);

	if (ImGui::CollapsingHeader("GPU Timings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		const float unitScaling = 1.0f / 1000.0f; //turn to miliseconds

		for (uint32_t i = 0; i < timeStamps.size(); i++)
		{
			float value = timeStamps[i].microseconds * unitScaling;

			std::string name = timeStamps[i].label;
			if (name.length() >= 24)
			{
				name = name.substr(0, 22) + "...";
			}

			ImGui::Text("%-27s: %7.2f %s", name.c_str(), value, "ms");
		}
	}

	ImGui::End(); // PROFILER
}

void Renderer::ImGuiTextureDebugger()
{
	ImGui::Begin("Texture Debugger");

	auto& texturesMap = m_ResourceManager.GetTextures();
	std::vector<std::pair<uint32_t, VulkanTexture*>> textures(texturesMap.begin(), texturesMap.end());
	std::sort(
		textures.begin(), 
		textures.end(), 
		[](const std::pair<uint32_t, VulkanTexture*>& a, const std::pair<uint32_t, VulkanTexture*>& b) 
		{
			if (a.second != nullptr && b.second != nullptr)
				return a.second->GetName() < b.second->GetName();
			else
				return false;
		}
	);

	if (ImGui::BeginCombo("##combo", currentTextureSelectedName.c_str())) // The second parameter is the label previewed before opening the combo.
	{
		for (auto& textureIdPair : textures)
		{
			auto currentTexture = textureIdPair.second;

			if (currentTexture == nullptr)
				continue;

			auto currentId = textureIdPair.first;
			std::string currentTextureName = currentTexture->GetName();

			bool is_selected = (currentTextureSelectedID == currentId); // You can store your selection however you want, outside or inside your objects
			if (ImGui::Selectable(currentTextureName.c_str(), is_selected))
			{
				currentTextureSelectedName = currentTextureName.c_str();
				currentTextureSelectedID = currentId;
			}
			if (is_selected)
			{
				ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
			}
		}
		ImGui::EndCombo();
	}

	VulkanTexture* currentSelectedTexture = GetTextureWithID(currentTextureSelectedID);

	if (currentSelectedTexture)
	{
		auto& debugTextureParams = TextureDebugPass::ParamsCPU;

		auto region = currentSelectedTexture->GetRegion();

		uint32_t arraySize = region.Subresource.ArraySize;
		bool isArray = arraySize > 1;

		bool is3D = region.Extent.Depth > 1;
		debugTextureParams.is3D = is3D;

		debugTextureParams.isArray = isArray;
		debugTextureParams.arrayCount = arraySize;

		if (isArray)
		{
			ImGui::SliderInt("Array Slice: ", &debugTextureParams.arraySlice, 0, arraySize - 1);
		}
		else if (is3D)
		{
			ImGui::SliderFloat("Depth Scale: ", &debugTextureParams.texture3DDepthScale, 0.f, 1.f);
		}

		static bool r = true, g = true, b = true, a = true;
		ImGui::Checkbox("R:", &r);
		ImGui::SameLine();
		ImGui::Checkbox("G:", &g);
		ImGui::SameLine();
		ImGui::Checkbox("B:", &b);
		ImGui::SameLine();
		ImGui::Checkbox("A:", &a);
		debugTextureParams.showR = r;
		debugTextureParams.showG= g;
		debugTextureParams.showB = b;
		debugTextureParams.showA = a;

		auto textureDebugerSize = ImGui::GetWindowSize();
		const float minTextureHeight = 600.f;
		const float textureInfoOffset = 100.f;

		float textureWidth = static_cast<float>(currentSelectedTexture->GetWidth());
		float textureHeight = static_cast<float>(currentSelectedTexture->GetHeight());

		ImGui::Image(m_ImGuiManager.GetImGuiTextureFrom(TextureDebugPass::Output), GetScaledSizeWithAspectRatioKept(ImVec2(textureWidth, textureHeight), 800.f));
	}

	ImGui::End();
}

ImVec2 Renderer::GetScaledSizeWithAspectRatioKept(ImVec2 currentSize, float minWidth)
{
	float inverseAspectRatio = (currentSize.y / float(currentSize.x));

	float texWidth = std::max(currentSize.x, minWidth);
	float texHeight = texWidth * inverseAspectRatio;

	ImVec2 currentWindowSize = ImGui::GetWindowSize();

	if (currentWindowSize.x < texWidth || currentWindowSize.y < texHeight)
	{
		
		float newTexWidth = currentWindowSize.x;
		float newTexHeight = newTexWidth * inverseAspectRatio;

		if (currentWindowSize.y < newTexHeight)
		{
			newTexWidth = currentWindowSize.y * 1.f / inverseAspectRatio;
			newTexHeight = currentWindowSize.y;
		}

		return ImVec2(newTexWidth, newTexHeight);
	}
	else
	{
		return ImVec2(texWidth, texHeight);
	}

}

void Renderer::BindViewport(float x, float y, float width, float height)
{
	VkViewport viewport;
	viewport.x = x;
	viewport.y = y;
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = { static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height) };

	vkCmdSetViewport(GET_VK_HANDLE(GetCurrentCommandBuffer()), 0, 1, &viewport);
	vkCmdSetScissor(GET_VK_HANDLE(GetCurrentCommandBuffer()), 0, 1, &scissor);
}

void Renderer::BindVertexData(size_t offset)
{
	VkBuffer buffers[] = { GET_VK_HANDLE_PTR(m_VertexUploadBuffer) };
	VkDeviceSize offsets[] = { (VkDeviceSize)offset };
	vkCmdBindVertexBuffers(GET_VK_HANDLE(GetCurrentCommandBuffer()), 0, 1, buffers, offsets);
}

void Renderer::DrawVertices(uint32_t count)
{
	BindPipeline<GraphicsPipeline>();

	m_StateManager.GetRenderPass()->BeginRenderPass(GetCurrentCommandBuffer());

	vkCmdDraw(GET_VK_HANDLE(GetCurrentCommandBuffer()), count, 1, 0, 0);

	m_StateManager.GetRenderPass()->EndRenderPass(GetCurrentCommandBuffer());
	
	m_StateManager.Reset();
}

void Renderer::Dispatch(uint32_t x, uint32_t y, uint32_t z)
{
	BindPipeline<ComputePipeline>();

	vkCmdDispatch(GET_VK_HANDLE(GetCurrentCommandBuffer()), x, y, z);
}

void Renderer::ClearImage(VulkanTexture* texture, Color clearValue)
{
	auto currentState = texture->GetResourceState();
	m_VulkanDevice.ResourceBarrier(GetCurrentCommandBuffer(), texture, texture->GetResourceState(), ResourceState::TransferDst, texture->GetCurrentResourceStage(), ResourceStage::Transfer);

	VkClearColorValue clearColor{ clearValue.value.x, clearValue.value.y, clearValue.value.z, clearValue.value.w };
	VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, texture->GetMipCount(), 0, texture->GetRegion().Subresource.ArraySize };
	vkCmdClearColorImage(GET_VK_HANDLE(GetCurrentCommandBuffer()), GET_VK_HANDLE_PTR(texture->GetResource()), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);

	m_VulkanDevice.ResourceBarrier(GetCurrentCommandBuffer(), texture, ResourceState::TransferDst, currentState, ResourceStage::Transfer, texture->GetPreviousResourceStage());
}

void Renderer::CopyImageToBuffer(VulkanTexture* texture, VulkanBuffer* buffer)
{
	m_VulkanDevice.CopyImageToBuffer(GetCurrentCommandBuffer(), texture, buffer);
}

void Renderer::CopyBufferToImage(VulkanBuffer* buffer, VulkanTexture* texture)
{
	m_VulkanDevice.CopyBufferToImage(GetCurrentCommandBuffer(), buffer, texture);
}

void Renderer::CopyImage(VulkanTexture* src, VulkanTexture* dst)
{
	m_VulkanDevice.CopyImage(GetCurrentCommandBuffer(), src, dst);
}

void Renderer::CopyBuffer(VulkanBuffer& src, VulkanBuffer& dst, uint64_t size /*= UINT64_MAX*/, uint64_t srcOffset /*= 0*/, uint64_t dstOffset /*= 0*/)
{
	// if size is not set, we assume that we want to copy whole src buffer
	if (size == UINT64_MAX)
	{
		size = src.GetSize();
	}

	m_VulkanDevice.CopyBuffer(GetCurrentCommandBuffer(), src, dst, size, srcOffset, dstOffset);
}

IndexedIndirectBuffer::IndexedIndirectBuffer(VulkanDevice& device, uint32_t numCommands)
	: gpuBuffer(device, BufferCreateInfo{
		.flags = BufferUsageFlags::IndirectBuffer | BufferUsageFlags::TransferSrc,
		.memoryAccess = MemoryAccess::CPU2GPU,
		.size = sizeof(IndexIndirectDrawData) * numCommands,
		.name = "GeomDataIndirectDraw" })
{
	localBuffer = RABBIT_ALLOC(IndexIndirectDrawData, numCommands);
}

IndexedIndirectBuffer::~IndexedIndirectBuffer()
{
	RABBIT_FREE(localBuffer);
}

void IndexedIndirectBuffer::SubmitToGPU()
{
	gpuBuffer.FillBuffer(localBuffer, currentOffset * sizeof(IndexIndirectDrawData));
}


void IndexedIndirectBuffer::AddIndirectDrawCommand(VulkanCommandBuffer& commandBuffer, IndexIndirectDrawData& drawData)
{
    vkCmdDrawIndexedIndirect(
		GET_VK_HANDLE(commandBuffer), 
		GET_VK_HANDLE(gpuBuffer), 
		currentOffset * sizeof(IndexIndirectDrawData), 
		1, 
		sizeof(IndexIndirectDrawData));
	
	localBuffer[currentOffset] = drawData;
	currentOffset++;
}

void IndexedIndirectBuffer::Reset()
{
	currentOffset = 0;
}

template<>
void Renderer::BindPipeline<GraphicsPipeline>()
{
	m_ResourceStateTrackingManager.CommitBarriers(*this);

	//renderpass
	std::vector<VulkanImageView*>& attachments = m_StateManager.GetRenderTargets();
	VulkanImageView* depthStencil = m_StateManager.GetDepthStencil();
	RenderPassInfo* renderPassInfo = m_StateManager.GetRenderPassInfo();

	RenderPass* renderpass =
		m_StateManager.GetRenderPassDirty()
		? m_PipelineManager.FindOrCreateRenderPass(m_VulkanDevice, attachments, depthStencil, *renderPassInfo)
		: m_StateManager.GetRenderPass();

	m_StateManager.SetRenderPass(renderpass);

	//pipeline
	PipelineInfo* pipelineInfo = m_StateManager.GetPipelineInfo();

	pipelineInfo->renderPass = &m_StateManager.GetRenderPass()->GetVulkanRenderPass();
	VulkanPipeline* pipeline =
		m_StateManager.GetPipelineDirty()
		? m_PipelineManager.FindOrCreateGraphicsPipeline(m_VulkanDevice, *pipelineInfo)
		: m_StateManager.GetPipeline();

	m_StateManager.SetPipeline(pipeline);

	pipeline->Bind(GetCurrentCommandBuffer());

	BindDescriptorSets();

	BindPushConstInternal();
}

template<>
void Renderer::BindPipeline<ComputePipeline>()
{
	m_ResourceStateTrackingManager.CommitBarriers(*this);

	PipelineInfo* pipelineInfo = m_StateManager.GetPipelineInfo();

	VulkanPipeline* computePipeline = m_StateManager.GetPipelineDirty()
		? m_PipelineManager.FindOrCreateComputePipeline(m_VulkanDevice, *pipelineInfo)
		: m_StateManager.GetPipeline();

	m_StateManager.SetPipeline(computePipeline);

	computePipeline->Bind(GetCurrentCommandBuffer());

	BindDescriptorSets();

	BindPushConstInternal();
}

template<>
void Renderer::BindPipeline<RayTracingPipeline>()
{
	m_ResourceStateTrackingManager.CommitBarriers(*this);

	PipelineInfo* pipelineInfo = m_StateManager.GetPipelineInfo();

	VulkanPipeline* rayTracingPipeline = m_StateManager.GetPipelineDirty()
		? m_PipelineManager.FindOrCreateRayTracingPipeline(m_VulkanDevice, *pipelineInfo)
		: m_StateManager.GetPipeline();

	m_StateManager.SetPipeline(rayTracingPipeline);

	rayTracingPipeline->Bind(GetCurrentCommandBuffer());

	BindDescriptorSets();

	BindPushConstInternal();
}
