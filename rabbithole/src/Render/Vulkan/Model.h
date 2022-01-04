#pragma once

#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <vector>
#include <type_traits>
#include <unordered_map>
#include <glm/gtx/hash.hpp>

#include "../Model/ModelLoading.h"

class VulkanImage;
class VulkanImageView;
class VulkanImageSampler;

void InitDefaultTextures(VulkanDevice* device);

struct SimplePushConstantData
{
	rabbitMat4f modelMatrix;
	uint32_t	id;
};

struct Vertex
{
	rabbitVec3f position;
	rabbitVec3f normal;
	rabbitVec3f tangent;
	rabbitVec2f uv;

	static std::vector<VkVertexInputBindingDescription>		GetBindingDescriptions();
	static std::vector<VkVertexInputAttributeDescription>	GetAttributeDescriptions();

	bool operator==(const Vertex& other) const
	{
		return position == other.position && normal == other.normal && tangent == other.tangent && uv == other.uv;
	}
};

template <>
struct std::hash<Vertex>
{
	size_t operator()(const Vertex& k) const
	{
		size_t res = 17;
		res = res * 31 + std::hash<rabbitVec3f>()(k.position);
		res = res * 31 + std::hash<rabbitVec3f>()(k.normal);
		res = res * 31 + std::hash<rabbitVec2f>()(k.tangent);
		res = res * 31 + std::hash<rabbitVec2f>()(k.uv);
		return res;
	}
};

struct TextureData
{
	unsigned char* pData;
	int width;
	int height;
	int bpp;
};

struct Mesh
{
	rabbitVec3f position = { 0.0, 0.0, 0.0 };
	rabbitVec3f rotation = { 0.0, 0.0, 0.0 };
	rabbitVec3f scale = { 1.f, 1.f, 1.f };
	rabbitMat4f modelMatrix = rabbitMat4f{ 1.f };

	void CalculateMatrix();
};

class RabbitModel
{
public:

	RabbitModel(VulkanDevice& device, std::string filepath, std::string name);
	RabbitModel(VulkanDevice& device, ModelLoading::ObjectData* objectData);
	~RabbitModel();

	RabbitModel(const RabbitModel&) = delete;
	RabbitModel& operator=(const RabbitModel&) = delete;

	VulkanTexture*			GetAlbedoTexture()	const { return m_AlbedoTexture; }
	VulkanTexture*			GetNormalTexture()	const { return m_NormalTexture; }
	VulkanTexture*			GetRoughnessTexture() const { return m_RoughnessTexture; }
	VulkanTexture*			GetMetalnessTexture() const { return m_MetalnessTexture; }

	VulkanDescriptorSet*	GetDescriptorSet() const { return m_DescriptorSet; }
	void					SetDescriptorSet(VulkanDescriptorSet* ds) { m_DescriptorSet = ds; }

	void Bind(VkCommandBuffer commandBuffer);
	void Draw(VkCommandBuffer commandBuffer);

	void LoadFromFile();
	
	//test purpose, make it private

	Mesh GetMesh() { return m_MeshData; }
	void SetMesh(Mesh mesh) { m_MeshData = mesh; }
	inline uint32_t GetId() const { return m_Id; }

private:
	void CreateTextures(ModelLoading::MaterialData* material);
	void CreateVertexBuffers();
	void CreateIndexBuffers();

	VulkanDevice&			m_VulkanDevice;
	VulkanBuffer*			m_VertexBuffer;
	VulkanBuffer*			m_IndexBuffer;
	std::vector<Vertex>		m_Vertices;
	std::vector<uint32_t>	m_Indices;
	uint32_t				m_VertexCount;
	uint32_t				m_IndexCount;
	bool					hasIndexBuffer = false;

	TextureData*			m_TextureData{};
	VulkanTexture*			m_AlbedoTexture;
	VulkanTexture*			m_NormalTexture;
	VulkanTexture*			m_RoughnessTexture;
	VulkanTexture*			m_MetalnessTexture;
	VulkanDescriptorSet*	m_DescriptorSet;

	Mesh					m_MeshData;
public:
	static VulkanTexture*	ms_DefaultWhiteTexture;
	static uint32_t			m_CurrentId;
private:
	std::string				m_FilePath{};
	std::string				m_Name{};
	uint32_t				m_Id;
};



struct SceneNode 
{
	int mesh, material;
	int parent;
	int firstChild;
	int rightSibling;
};

struct Hierarchy 
{
	int parent;
	int firstChild;
	int nextSibling;
	int level;
};

struct Scene 
{
	std::vector<rabbitMat4f> localTransforms;
	std::vector<rabbitMat4f> globalTransforms;
	std::vector<Hierarchy> hierarchy;
	// Meshes for nodes (Node -> Mesh)
	std::unordered_map<uint32_t, uint32_t> meshes;
	// Materials for nodes (Node -> Material)
	std::unordered_map<uint32_t, uint32_t> materialForNode;
	// Node names: which name is assigned to the node
	std::unordered_map<uint32_t, uint32_t> nameForNode;
	// Collection of scene node names
	std::vector<std::string> names;
	// Collection of debug material names
	std::vector<std::string> materialNames;
};