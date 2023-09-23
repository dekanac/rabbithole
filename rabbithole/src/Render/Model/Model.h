#pragma once

#include "Render/Vulkan/VulkanDevice.h"
#include "Render/Vulkan/VulkanBuffer.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <vector>
#include <type_traits>
#include <unordered_map>
#include <glm/gtx/hash.hpp>
#include "tinygltf/tiny_gltf.h"

#include <Assimp/Importer.hpp>
#include <Assimp/scene.h>
#include <Assimp/postprocess.h>

#include "TextureLoading.h"

class VulkanImage;
class VulkanImageView;
class VulkanImageSampler;
class VulkanDescriptorSet;
class VulkanPipelineLayout;
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

struct SimplePushConstantData
{
	rabbitMat4f modelMatrix;
	uint32_t	id;
	uint32_t	useAlbedoMap;
	uint32_t	useNormalMap;
	uint32_t	useMetallicRoughnessMap;
	rabbitVec4f baseColor;
	rabbitVec4f emmisiveColorAndStrength;
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

struct AABB
{
	rabbitVec3f bounds[2];
	inline rabbitVec3f centroid() const { return (bounds[0] + bounds[1]) * 0.5f; }
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

using TextureData = TextureLoading::TextureData;

enum RenderingContext
{
	GBuffer_Opaque,
	Clouds_Transparent
};

class VulkanglTFModel
{
public:
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
		glm::mat4 matrix;
		AABB bbox;
	};

	struct Material 
	{
		glm::vec4	baseColorFactor = glm::vec4(1.0f);
		glm::vec4   emissiveColorAndStrenght = glm::vec4(0.0f);
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

	void LoadImages(tinygltf::Model& input);
	void LoadTextures(tinygltf::Model& input);
	void LoadMaterials(tinygltf::Model& input);
	void LoadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, VulkanglTFModel::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);
	void LoadModelFromFile(std::string& filename);
	void CreateDescriptorSet(uint32_t imageIndex);

public:
	void DrawNode(VulkanCommandBuffer& commandBuffer, const VulkanPipelineLayout* pipelineLayout, VulkanglTFModel::Node node, uint8_t backBufferIndex, IndexedIndirectBuffer* indirectBuffer);
	void Draw(VulkanCommandBuffer& commandBuffer, const VulkanPipelineLayout* pipeLayout, uint8_t backBufferIndex, IndexedIndirectBuffer* indirectBuffer);
	void BindBuffers(VulkanCommandBuffer& commandBuffer);
};