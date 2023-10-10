#pragma once

#include "Render/Model/TextureLoading.h"
#include "Render/Vulkan/VulkanCommandBuffer.h"

#include <vector>
#include <type_traits>
#include <unordered_map>
#include <glm/gtx/hash.hpp>

class VulkanDescriptorSet;
class VulkanPipelineLayout;
class VulkanBuffer;
struct IndexedIndirectBuffer;
class Renderer;

struct IndexIndirectDrawData
{
	uint32_t    indexCount;
	uint32_t    instanceCount;
	uint32_t    firstIndex;
	int32_t     vertexOffset;
	uint32_t    firstInstance;
};

struct ModelDrawData
{
	Matrix44f	modelMatrix;
	uint32_t	id;
	uint32_t	useAlbedoMap;
	uint32_t	useNormalMap;
	uint32_t	useMetallicRoughnessMap;
	Vector4f	baseColor;
	Vector4f	emmisiveColorAndStrength;
};

struct Vertex
{
	Vector3f position;
	Vector3f normal;
	Vector3f tangent;
	Vector2f uv;

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
		res = res * 31 + std::hash<Vector3f>()(k.position);
		res = res * 31 + std::hash<Vector3f>()(k.normal);
		res = res * 31 + std::hash<Vector2f>()(k.tangent);
		res = res * 31 + std::hash<Vector2f>()(k.uv);
		return res;
	}
};

enum RenderingContext
{
	GBuffer_Opaque,
	Clouds_Transparent
};

class VulkanglTFModel
{
public:
	struct AABB
	{
		Vector3f bounds[2];
		inline Vector3f centroid() const { return (bounds[0] + bounds[1]) * 0.5f; }
	};

	VulkanglTFModel(Renderer* renderer, std::string filename, RenderingContext context, bool flipNormalY = false);
	~VulkanglTFModel();

	static uint32_t ms_CurrentDrawId;
	
	VulkanglTFModel(const VulkanglTFModel& other) = delete;
	VulkanglTFModel(VulkanglTFModel&& other) = default;
private:
	Renderer*			m_Renderer;
	std::string			m_Path;
	bool				m_FlipNormalY;
	RenderingContext	m_RenderingContext;

	VulkanBuffer*	m_VertexBuffer;
	VulkanBuffer*	m_IndexBuffer;
	uint32_t		m_IndexCount;

public:
	inline VulkanBuffer*	GetVertexBuffer() const	{ return m_VertexBuffer; }
	inline VulkanBuffer*	GetIndexBuffer() const { return m_IndexBuffer; }
	uint32_t				GetIndexCount()		{ return m_IndexCount; }

private:
	// A primitive contains the data for a single draw call
	struct Primitive 
	{
		uint32_t firstIndex;
		uint32_t indexCount;
		int32_t	 materialIndex;
	};

	struct Mesh 
	{
		std::vector<Primitive> primitives;
	};

public:
	struct Node 
	{
		Node* parent;
		std::vector<Node> children;
		Mesh mesh;
		Matrix44f matrix;
		AABB bbox;
	};

	struct Material 
	{
		Vector4f	baseColorFactor = Vector4f(1.0f);
		Vector4f   emissiveColorAndStrenght = Vector4f(0.0f);
		int32_t		baseColorTextureIndex = -1;
		int32_t		normalTextureIndex = -1;
		int32_t		metallicRoughnessTextureIndex = -1;

		//TODO: split descriptors into 3 levels PER_OBJECT, PER_PASS and PER_FRAME, then you should get rid of this
		VulkanDescriptorSet* materialDescriptorSet[MAX_FRAMES_IN_FLIGHT];
	};

private:
	std::vector<VulkanTexture*>		m_Textures;
	std::vector<uint32_t>			m_TextureIndices;
	std::vector<Material>			m_Materials;
	std::vector<Node>				m_Nodes;

public:
	std::vector<Node>&				GetNodes() { return m_Nodes; }
	std::vector<VulkanTexture*>&	GetTextures() { return m_Textures; }
	std::vector<Material>&			GetMaterials() { return m_Materials; }
	std::vector<uint32_t>&			GetTextureIndices() { return m_TextureIndices; }

private:
	void LoadNode(const aiNode* inputNode, const aiScene* input, VulkanglTFModel::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);
	void LoadMaterials(const aiScene* input);
	void LoadModelFromFile(std::string& filename);
	void CreateDescriptorSet(uint32_t imageIndex);

public:
	void DrawNode(VulkanCommandBuffer& commandBuffer, const VulkanPipelineLayout* pipelineLayout, VulkanglTFModel::Node node, uint8_t backBufferIndex, IndexedIndirectBuffer* indirectBuffer);
	void Draw(VulkanCommandBuffer& commandBuffer, const VulkanPipelineLayout* pipeLayout, uint8_t backBufferIndex, IndexedIndirectBuffer* indirectBuffer);
	void BindBuffers(VulkanCommandBuffer& commandBuffer);
};