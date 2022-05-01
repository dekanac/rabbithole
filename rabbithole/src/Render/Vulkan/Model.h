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

using TextureData = ModelLoading::TextureData;

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
	inline bool				GetUseNormalMap()   const { return m_UseNormalMap; }
	VulkanTexture*			GetRoughnessTexture() const { return m_RoughnessTexture; }
	VulkanTexture*			GetMetalnessTexture() const { return m_MetalnessTexture; }

	const std::vector<Vertex>& GetVertices() const { return m_Vertices; }
	const std::vector<uint32_t>& GetIndices() const { return m_Indices; }

	VulkanDescriptorSet*	GetDescriptorSet() const { return m_DescriptorSet; }
	void					SetDescriptorSet(VulkanDescriptorSet* ds) { m_DescriptorSet = ds; }

	inline uint32_t GetIndexCount() { return m_IndexCount; }

	void Bind(VkCommandBuffer commandBuffer);

	void LoadFromFile();
	
	Mesh GetMesh()					{ return m_MeshData; }
	void SetMesh(Mesh mesh)			{ m_MeshData = mesh; }
	inline uint32_t GetId() const	{ return m_Id; }

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

	VulkanTexture*			m_AlbedoTexture;
	VulkanTexture*			m_NormalTexture;
	bool					m_UseNormalMap = true;
	VulkanTexture*			m_RoughnessTexture;
	VulkanTexture*			m_MetalnessTexture;
	VulkanDescriptorSet*	m_DescriptorSet;

	Mesh					m_MeshData;
public:
	AABB					m_BoundingBox{};
	static VulkanTexture*	ms_DefaultWhiteTexture;
	static VulkanTexture*	ms_DefaultBlackTexture;
	static uint32_t			m_CurrentId;
private:
	std::string				m_FilePath{};
	std::string				m_Name{};
	uint32_t				m_Id;

//BVH move to separate class
private:
	void CalculateAABB();


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
		:
		bottom(FLT_MAX, FLT_MAX, FLT_MAX),
		top(-FLT_MAX, -FLT_MAX, -FLT_MAX),
		pTri(NULL)
	{}
};
typedef std::vector<BBoxTmp> BBoxEntries;  // vector of triangle bounding boxes needed during BVH construction

BVHNode* Recurse(BBoxEntries& work, int depth = 0);
BVHNode* CreateBVH(std::vector<rabbitVec4f>& vertices, std::vector<Triangle>& triangles);

struct CacheFriendlyBVHNode {
	// bounding box
	rabbitVec3f bottom;
	uint32_t pad1;
	rabbitVec3f top;
	

	// parameters for leafnodes and innernodes occupy same space (union) to save memory
	// top bit discriminates between leafnode and innernode
	// no pointers, but indices (int): faster

	union {
		// inner node - stores indexes to array of CacheFriendlyBVHNode
		struct {
			uint32_t idxLeft;
			uint32_t idxRight;
			uint32_t isLeaf;
		} inner;
		// leaf node: stores triangle count and starting index in triangle list
		struct {
			uint32_t count; // Top-most bit set, leafnode if set, innernode otherwise
			uint32_t startIndexInTriIndexList;
			uint32_t isLeaf;
		} leaf;
	} u;

	uint32_t pad2[2];
};

// The ugly, cache-friendly form of the BVH: 32 bytes
void CreateCFBVH(const Triangle* triangles, BVHNode* rootBVH, uint32_t** triIndexList, uint32_t* triIndexListNum, CacheFriendlyBVHNode** nodeList, uint32_t* nodeListNum);
int CountBoxes(BVHNode* root);
unsigned CountTriangles(BVHNode* root);
void CountDepth(BVHNode* root, int depth, int& maxDepth);
void PopulateCacheFriendlyBVH(const Triangle* pFirstTriangle, BVHNode* root, unsigned& idxBoxes, unsigned& idxTriList, uint32_t* triIndexList, CacheFriendlyBVHNode* nodeList);