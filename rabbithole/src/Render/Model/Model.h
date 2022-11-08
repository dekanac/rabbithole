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

#include "TextureLoading.h"

class VulkanImage;
class VulkanImageView;
class VulkanImageSampler;
class VulkanDescriptorSet;
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
	bool		useNormalMap;
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

struct Triangle
{
	int indices[3];
};

struct BVHNode 
{
	rabbitVec3f bottom;
	rabbitVec3f top;
	virtual bool IsLeaf() = 0;
};

struct BVHInner : BVHNode 
{
	BVHNode* left;
	BVHNode* right;
	virtual bool IsLeaf() { return false; }
};

struct BVHLeaf : BVHNode 
{
	std::list<const Triangle*> triangles;
	virtual bool IsLeaf() { return true; }
};

struct BBoxTmp
{
	rabbitVec3f bottom;
	rabbitVec3f top;
	rabbitVec3f center;

	const Triangle* pTri;

	BBoxTmp()
		: bottom(FLT_MAX, FLT_MAX, FLT_MAX)
		, top(-FLT_MAX, -FLT_MAX, -FLT_MAX)
		, pTri(NULL)
	{}
};
typedef std::vector<BBoxTmp> BBoxEntries;  // vector of triangle bounding boxes needed during BVH construction

BVHNode* Recurse(BBoxEntries& work, int depth = 0);
BVHNode* CreateBVH(std::vector<rabbitVec4f>& vertices, std::vector<Triangle>& triangles);

struct CacheFriendlyBVHNode 
{
	// bounding box
	rabbitVec3f bottom;
	rabbitVec3f top;

	// parameters for leafnodes and innernodes occupy same space (union) to save memory
	// top bit discriminates between leafnode and innernode
	// no pointers, but indices (int): faster

	union 
	{
		// inner node - stores indexes to array of CacheFriendlyBVHNode
		struct 
		{
			uint32_t idxLeft;
			uint32_t idxRight;
		} inner;
		// leaf node: stores triangle count and starting index in triangle list
		struct 
		{
			uint32_t count; // Top-most bit set, leafnode if set, innernode otherwise
			uint32_t startIndexInTriIndexList;
		} leaf;
	} u;
};

// The ugly, cache-friendly form of the BVH: 32 bytes
void CreateCFBVH(const Triangle* triangles, BVHNode* rootBVH, uint32_t** triIndexList, uint32_t* triIndexListNum, CacheFriendlyBVHNode** nodeList, uint32_t* nodeListNum);
int CountBoxes(BVHNode* root);
unsigned CountTriangles(BVHNode* root);
void CountDepth(BVHNode* root, int depth, int& maxDepth);
void PopulateCacheFriendlyBVH(const Triangle* pFirstTriangle, BVHNode* root, unsigned& idxBoxes, unsigned& idxTriList, uint32_t* triIndexList, CacheFriendlyBVHNode* nodeList);

class VulkanglTFModel
{
public:
	VulkanglTFModel(Renderer* renderer, std::string filename);
	~VulkanglTFModel();
	
	VulkanglTFModel(const VulkanglTFModel& other) = delete;
	VulkanglTFModel(VulkanglTFModel&& other) = default;
private:
	Renderer*		m_Renderer;

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
		uint32_t	baseColorTextureIndex;
		uint32_t	normalTextureIndex;
		uint32_t	metallicRoughnessTextureIndex;

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
	void LoadImages(tinygltf::Model& input);
	void LoadTextures(tinygltf::Model& input);
	void LoadMaterials(tinygltf::Model& input);
	void LoadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, VulkanglTFModel::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);
	void LoadModelFromFile(std::string filename);

public:
	void DrawNode(VulkanCommandBuffer& commandBuffer, const VkPipelineLayout* pipelineLayout, VulkanglTFModel::Node node, uint8_t backBufferIndex, IndexedIndirectBuffer* indirectBuffer);
	void Draw(VulkanCommandBuffer& commandBuffer, const VkPipelineLayout* pipeLayout, uint8_t backBufferIndex, IndexedIndirectBuffer* indirectBuffer);
	void BindBuffers(VulkanCommandBuffer& commandBuffer);
};