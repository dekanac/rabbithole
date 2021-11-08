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

struct SimplePushConstantData
{
	rabbitMat4f modelMatrix;
};

struct Vertex
{
	rabbitVec3f position;
	rabbitVec3f normal;
	rabbitVec2f uv;

	static std::vector<VkVertexInputBindingDescription>		GetBindingDescriptions();
	static std::vector<VkVertexInputAttributeDescription>	GetAttributeDescriptions();

	bool operator==(const Vertex& other) const
	{
		return position == other.position && normal == other.normal && uv == other.uv;
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

class RabbitModel
{
public:

	RabbitModel(VulkanDevice& device, std::string filepath, std::string name);
	RabbitModel(VulkanDevice& device, ModelLoading::ObjectData* objectData);
	~RabbitModel();

	RabbitModel(const RabbitModel&) = delete;
	RabbitModel& operator=(const RabbitModel&) = delete;

	VulkanTexture* GetTexture()	const { return m_Texture; }

	rabbitMat4f	GetModelMatrix() { return m_ModelMatrix; }
	void SetModelMatrix(rabbitMat4f modelMatrix) { m_ModelMatrix = modelMatrix; }

	void Bind(VkCommandBuffer commandBuffer);
	void Draw(VkCommandBuffer commandBuffer);

	void LoadFromFile();

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
	VulkanTexture*			m_Texture;

	rabbitMat4f				m_ModelMatrix;

private:
	std::string				m_FilePath{};
	std::string				m_Name{};
};

class Mesh
{
public:
	std::vector<Vertex>		GetVertices() { return m_Vertices; }
	std::vector<uint32_t>	GetIndices() { return m_Indices; }

	uint32_t GetIndexCount() { return m_IndexCount; }
	uint32_t GetVertexCount() { return m_VertexCount; }

private:
	std::vector<Vertex>		m_Vertices;
	std::vector<uint32_t>	m_Indices;

	uint32_t				m_VertexCount;
	uint32_t				m_IndexCount;
	bool					hasIndexBuffer = false;

};

class Material
{
public:
	//use default gray texture instead nullptr
	VulkanTexture* GetDiffuseTexture() { return m_DiffuseTexture ? m_DiffuseTexture : nullptr; }
private:
	VulkanTexture* m_DiffuseTexture;
};

struct TransformData
{
	rabbitMat4f modelMatrix = ZERO_MAT4f;
	rabbitVec3f modelPosition = ZERO_VEC3f;
	rabbitVec3f modelRotation = ZERO_VEC3f;

	float modelScale = 1.f;
};

class SceneObject
{

public:
	Mesh*			GetMesh() { return m_Mesh; };
	Material*		GetMaterial() { return m_Material; }

private:
	Mesh*			m_Mesh;
	Material*		m_Material;
	TransformData*	m_TransformData;

	uint32_t		m_ID;
};

