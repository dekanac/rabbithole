#include "precomp.h"

#include <stddef.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader/tiny_obj_loader.h"
#include "stb_image/stb_image.h"

VulkanTexture* RabbitModel::ms_DefaultWhiteTexture = nullptr;
VulkanTexture* RabbitModel::ms_DefaultBlackTexture = nullptr;

uint32_t RabbitModel::m_CurrentId = 1;

void RabbitModel::CalculateAABB()
{
	rabbitVec3f max = m_Vertices[0].position;
	rabbitVec3f min = max;

	for (int i = 1; i < m_VertexCount; i++)
	{
		max = glm::max(max, m_Vertices[i].position);
		min = glm::min(min, m_Vertices[i].position);
	}

	m_BoundingBox.bounds[0] = min;
	m_BoundingBox.bounds[1] = max;
}

RabbitModel::RabbitModel(VulkanDevice& device, std::string filepath, std::string name) 
	: m_VulkanDevice{ device }
	, m_FilePath(filepath)
	, m_Name(name)
	, m_Id(m_CurrentId)
{
	m_CurrentId++;

	LoadFromFile();
	//CreateTextures();
	CreateVertexBuffers();
	CreateIndexBuffers();

	CalculateAABB();
}

RabbitModel::RabbitModel(VulkanDevice& device, ModelLoading::ObjectData* objectData)
	: m_VulkanDevice{ device }
	, m_Id(m_CurrentId)
{
	m_CurrentId++;

	//TODO:[MEMLEAK] get rid of this unneccessary shit boi, free MeshData 
	m_Vertices.resize(objectData->mesh->numVertices);
	m_Indices.resize(objectData->mesh->numIndices);
	m_VertexCount = objectData->mesh->numVertices;
	std::memcpy(m_Vertices.data(), objectData->mesh->pVertices, objectData->mesh->numVertices * sizeof(ModelLoading::MeshVertex));
	m_IndexCount = objectData->mesh->numIndices;
	std::memcpy(m_Indices.data(), objectData->mesh->pIndices, objectData->mesh->numIndices * sizeof(uint32_t));
	
	CreateTextures(&objectData->material);
	CreateVertexBuffers();
	CreateIndexBuffers();

	CalculateAABB();
 }

RabbitModel::~RabbitModel()
{
	delete m_VertexBuffer;
	if (hasIndexBuffer && m_IndexBuffer != nullptr) 
	{
		delete m_IndexBuffer;
	}
	if (m_DescriptorSet)
	{
		delete m_DescriptorSet;
	}
}

void RabbitModel::CreateTextures(ModelLoading::MaterialData* material)
{

	if (!material->diffuseMap->INVALID && !(material->diffuseMap == nullptr))
	{														
		m_AlbedoTexture = new VulkanTexture(&m_VulkanDevice, material->diffuseMap, TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst, Format::R8G8B8A8_UNORM_SRGB, "Albedo");
	}
	else
	{
		m_AlbedoTexture = ms_DefaultWhiteTexture;
	}

	if (material->normalMap)
	{												
		m_NormalTexture = new VulkanTexture(&m_VulkanDevice, material->normalMap, TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst, Format::R8G8B8A8_UNORM, "Normal");
	}
	else
	{
		m_NormalTexture = ms_DefaultWhiteTexture;
		m_UseNormalMap = false;
	}

	if (material->roughnessMap)
	{													
		m_RoughnessTexture = new VulkanTexture(&m_VulkanDevice, material->roughnessMap, TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst, Format::R8G8B8A8_UNORM, "Roughness");
	}
	else
	{
		m_RoughnessTexture = ms_DefaultBlackTexture;
	}

	if (material->metallicMap)
	{
															
		m_MetalnessTexture = new VulkanTexture(&m_VulkanDevice, material->metallicMap, TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst, Format::R8G8B8A8_UNORM, "Metalness");
	}
	else
	{
		m_MetalnessTexture = ms_DefaultBlackTexture;
	}
}

void RabbitModel::CreateVertexBuffers()
{
	m_VertexCount = static_cast<uint32_t>(m_Vertices.size());
	ASSERT(m_VertexCount >= 3, "Vertex count must be greater then 3!");

	uint64_t bufferSize = sizeof(m_Vertices[0]) * m_VertexCount;

	VulkanBuffer* stagingBuffer = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::TransferSrc, MemoryAccess::CPU, bufferSize, "StagingBuffer");

	stagingBuffer->FillBuffer(m_Vertices.data(), static_cast<size_t>(bufferSize));

	m_VertexBuffer = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::VertexBuffer, MemoryAccess::GPU, bufferSize, "VertexBuffer");

	m_VulkanDevice.CopyBuffer(stagingBuffer->GetBuffer(), m_VertexBuffer->GetBuffer(), bufferSize);

	delete stagingBuffer;
}

void RabbitModel::CreateIndexBuffers()
{
	m_IndexCount = static_cast<uint32_t>(m_Indices.size());
	hasIndexBuffer = m_IndexCount > 0;

	if (!hasIndexBuffer)
	{
		return;
	}

	VkDeviceSize bufferSize = sizeof(m_Indices[0]) * m_IndexCount;

	VulkanBuffer* stagingBuffer = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::TransferSrc, MemoryAccess::CPU, bufferSize, "StagingBuffer");

	stagingBuffer->FillBuffer(m_Indices.data(), static_cast<size_t>(bufferSize));

	m_IndexBuffer = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::IndexBuffer, MemoryAccess::GPU, bufferSize, "IndexBuffer");

	m_VulkanDevice.CopyBuffer(stagingBuffer->GetBuffer(), m_IndexBuffer->GetBuffer(), bufferSize);

	delete stagingBuffer;
}

void RabbitModel::LoadFromFile()
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes; // TODO: These are probably nodes???
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	ASSERT(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, m_FilePath.c_str()), "Cannot load obj file!");

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};

			vertex.position =
			{
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.normal =
			{
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]
			};

			vertex.uv =
			{
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(m_Vertices.size());
				m_Vertices.push_back(vertex);
			}

			m_Indices.push_back(uniqueVertices[vertex]);
		}
	}
}

void RabbitModel::Bind(VkCommandBuffer commandBuffer)
{
	VkBuffer buffers[] = { m_VertexBuffer->GetBuffer() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

	if (hasIndexBuffer)
	{
		vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
	}
}

std::vector<VkVertexInputBindingDescription> Vertex::GetBindingDescriptions()
{
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> Vertex::GetAttributeDescriptions()
{
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, position);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, normal);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, tangent);

	attributeDescriptions[3].binding = 0;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[3].offset = offsetof(Vertex, uv);

	return attributeDescriptions;
}

void InitDefaultTextures(VulkanDevice* device)
{
	RabbitModel::ms_DefaultWhiteTexture = new VulkanTexture(device, "res/textures/default_white.png", TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst, Format::R8G8B8A8_UNORM_SRGB, "Defaul_White");
	RabbitModel::ms_DefaultBlackTexture = new VulkanTexture(device, "res/textures/default_black.png", TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst, Format::R8G8B8A8_UNORM_SRGB, "Defaul_Black");
}

void Mesh::CalculateMatrix()
{
	float alpha = glm::radians(rotation.x);
	float beta = glm::radians(rotation.y);
	float gamma = glm::radians(rotation.z);

	float cosAlpha = glm::cos(alpha);
	float cosBeta = glm::cos(beta);
	float cosGamma = glm::cos(gamma);

	float sinAlpha = glm::sin(alpha);
	float sinBeta = glm::sin(beta);
	float sinGamma = glm::sin(gamma);

	modelMatrix[0][0] = cosAlpha * cosBeta;
	modelMatrix[0][1] = cosAlpha * sinBeta * sinGamma - sinAlpha * cosGamma;
	modelMatrix[0][2] = cosAlpha * sinBeta * cosGamma + sinAlpha * sinGamma;
	modelMatrix[1][0] = sinAlpha * cosBeta;
	modelMatrix[1][1] = sinAlpha * sinBeta * sinGamma + cosAlpha * cosGamma;
	modelMatrix[1][2] = sinAlpha * sinBeta * cosGamma - cosAlpha * sinGamma;
	modelMatrix[2][0] = -sinBeta;
	modelMatrix[2][1] = cosBeta * sinGamma;
	modelMatrix[2][2] = cosBeta * cosGamma;
	modelMatrix[3][0] = 0.f;
	modelMatrix[3][1] = 0.f;
	modelMatrix[3][2] = 0.f;
	modelMatrix[0][3] = position.x;
	modelMatrix[1][3] = position.y;
	modelMatrix[2][3] = position.z;
	modelMatrix[3][3] = 1.f;

	modelMatrix *= glm::vec4{scale.x, scale.y, scale.z, 1.f};

	modelMatrix = glm::transpose(modelMatrix);
}

BVHNode* Recurse(BBoxEntries& work, int depth)
{
	// terminate recursion case: 
	// if work set has less then 4 elements (triangle bounding boxes), create a leaf node 
	// and create a list of the triangles contained in the node

	if (work.size() < 4) {

		BVHLeaf* leaf = new BVHLeaf;
		for (BBoxEntries::iterator it = work.begin(); it != work.end(); it++)
			leaf->triangles.push_back(it->pTri);
		return leaf;
	}

	// else, work size > 4, divide  node further into smaller nodes
	// start by finding the working list's bounding box (top and bottom)

	rabbitVec3f bottom(FLT_MAX, FLT_MAX, FLT_MAX);
	rabbitVec3f top(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	// loop over all bboxes in current working list, expanding/growing the working list bbox
	for (unsigned i = 0; i < work.size(); i++) 
	{  // meer dan 4 bboxen in work
		BBoxTmp& v = work[i];
		bottom = glm::min(bottom, v.bottom);
		top = glm::max(top, v.top);
	}

	// SAH, surface area heuristic calculation
	// find surface area of bounding box by multiplying the dimensions of the working list's bounding box
	float side1 = top.x - bottom.x;  // length bbox along X-axis
	float side2 = top.y - bottom.y;  // length bbox along Y-axis
	float side3 = top.z - bottom.z;  // length bbox along Z-axis

	// the current bbox has a cost of (number of triangles) * surfaceArea of C = N * SA
	float minCost = work.size() * (side1 * side2 + side2 * side3 + side3 * side1);

	float bestSplit = FLT_MAX; // best split along axis, will indicate no split with better cost found (below)

	int bestAxis = -1;

	// Try all 3 axises X, Y, Z
	for (int j = 0; j < 3; j++) {  // 0 = X, 1 = Y, 2 = Z axis

		int axis = j;

		// we will try dividing the triangles based on the current axis,
		// and we will try split values from "start" to "stop", one "step" at a time.
		float start, stop, step;

		// X-axis
		if (axis == 0) {
			start = bottom.x;
			stop = top.x;
		}
		// Y-axis
		else if (axis == 1) {
			start = bottom.y;
			stop = top.y;
		}
		// Z-axis
		else {
			start = bottom.z;
			stop = top.z;
		}

		// In that axis, do the bounding boxes in the work queue "span" across, (meaning distributed over a reasonable distance)?
		// Or are they all already "packed" on the axis? Meaning that they are too close to each other
		if (fabsf(stop - start) < 1e-4)
			// BBox side along this axis too short, we must move to a different axis!
			continue; // go to next axis

		// Binning: Try splitting at a uniform sampling (at equidistantly spaced planes) that gets smaller the deeper we go:
		// size of "sampling grid": 1024 (depth 0), 512 (depth 1), etc
		// each bin has size "step"
		step = (stop - start) / (1024. / (depth + 1.));

		// for each bin (equally spaced bins of size "step"):
		for (float testSplit = start + step; testSplit < stop - step; testSplit += step) {

			// Create left and right bounding box
			rabbitVec3f lbottom(FLT_MAX, FLT_MAX, FLT_MAX);
			rabbitVec3f ltop(-FLT_MAX, -FLT_MAX, -FLT_MAX);
			rabbitVec3f rbottom(FLT_MAX, FLT_MAX, FLT_MAX);
			rabbitVec3f rtop(-FLT_MAX, -FLT_MAX, -FLT_MAX);

			// The number of triangles in the left and right bboxes (needed to calculate SAH cost function)
			int countLeft = 0, countRight = 0;

			// For each test split (or bin), allocate triangles in remaining work list based on their bbox centers
			// this is a fast O(N) pass, no triangle sorting needed (yet)
			for (unsigned i = 0; i < work.size(); i++) {

				BBoxTmp& v = work[i];

				// compute bbox center
				float value;
				if (axis == 0) value = v.center.x;       // X-axis
				else if (axis == 1) value = v.center.y;  // Y-axis
				else value = v.center.z;			   // Z-axis

				if (value < testSplit) {
					// if center is smaller then testSplit value, put triangle in Left bbox
					lbottom = glm::min(lbottom, v.bottom);
					ltop = glm::max(ltop, v.top);
					countLeft++;
				}
				else {
					// else put triangle in right bbox
					rbottom = glm::min(rbottom, v.bottom);
					rtop = glm::max(rtop, v.top);
					countRight++;
				}
			}

			// Now use the Surface Area Heuristic to see if this split has a better "cost"

			// First, check for stupid partitionings, ie bins with 0 or 1 triangles make no sense
			if (countLeft <= 1 || countRight <= 1) continue;

			// It's a real partitioning, calculate the surface areas
			float lside1 = ltop.x - lbottom.x;
			float lside2 = ltop.y - lbottom.y;
			float lside3 = ltop.z - lbottom.z;

			float rside1 = rtop.x - rbottom.x;
			float rside2 = rtop.y - rbottom.y;
			float rside3 = rtop.z - rbottom.z;

			// calculate SurfaceArea of Left and Right BBox
			float surfaceLeft = lside1 * lside2 + lside2 * lside3 + lside3 * lside1;
			float surfaceRight = rside1 * rside2 + rside2 * rside3 + rside3 * rside1;

			// calculate total cost by multiplying left and right bbox by number of triangles in each
			float totalCost = surfaceLeft * countLeft + surfaceRight * countRight;

			// keep track of cheapest split found so far
			if (totalCost < minCost) {
				minCost = totalCost;
				bestSplit = testSplit;
				bestAxis = axis;
			}
		} // end of loop over all bins
	} // end of loop over all axises

	// at the end of this loop (which runs for every "bin" or "sample location"), 
	// we should have the best splitting plane, best splitting axis and bboxes with minimal traversal cost

	// If we found no split to improve the cost, create a BVH leaf

	if (bestAxis == -1) {

		BVHLeaf* leaf = new BVHLeaf;
		for (BBoxEntries::iterator it = work.begin(); it != work.end(); it++)
			leaf->triangles.push_back(it->pTri); // put triangles of working list in leaf's triangle list
		return leaf;
	}

	// Otherwise, create BVH inner node with L and R child nodes, split with the optimal value we found above

	BBoxEntries left;
	BBoxEntries right;  // BBoxEntries is a vector/list of BBoxTmp 

	rabbitVec3f lbottom(FLT_MAX, FLT_MAX, FLT_MAX);
	rabbitVec3f ltop(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	rabbitVec3f rbottom(FLT_MAX, FLT_MAX, FLT_MAX);
	rabbitVec3f rtop(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	// distribute the triangles in the left or right child nodes
	// for each triangle in the work set
	for (int i = 0; i < (int)work.size(); i++) {

		// create temporary bbox for triangle
		BBoxTmp& v = work[i];

		// compute bbox center 
		float value;
		if (bestAxis == 0) value = v.center.x;
		else if (bestAxis == 1) value = v.center.y;
		else value = v.center.z;

		if (value < bestSplit) { // add temporary bbox v from work list to left BBoxentries list, 
			// becomes new working list of triangles in next step

			left.push_back(v);
			lbottom = glm::min(lbottom, v.bottom);
			ltop = glm::max(ltop, v.top);
		}
		else {

			// Add triangle bbox v from working list to right BBoxentries, 
			// becomes new working list of triangles in next step  
			right.push_back(v);
			rbottom = glm::min(rbottom, v.bottom);
			rtop = glm::max(rtop, v.top);
		}
	} // end loop for each triangle in working set

	// create inner node
	BVHInner* inner = new BVHInner;

	// recursively build the left child
	inner->left = Recurse(left, depth + 1);
	inner->left->bottom = lbottom;
	inner->left->top = ltop;

	// recursively build the right child
	inner->right = Recurse(right, depth + 1);
	inner->right->bottom = rbottom;
	inner->right->top = rtop;

	return inner;
}  // end of Recurse() function, returns the rootnode (when all recursion calls have finished)


BVHNode* CreateBVH(std::vector<rabbitVec4f>& vertices, std::vector<Triangle>& triangles)
{
	/* Summary:
	1. Create work BBox
	2. Create BBox for every triangle and compute bounds
	3. Expand bounds work BBox to fit all triangle bboxes
	4. Compute triangle bbox centre and add triangle to working list
	5. Build BVH tree with Recurse()
	6. Return root node
	*/

	std::vector<BBoxTmp> work;
	rabbitVec3f bottom(FLT_MAX, FLT_MAX, FLT_MAX);
	rabbitVec3f top(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	puts("Gathering bounding box info from all triangles...");
	// for each triangle
	for (unsigned j = 0; j < triangles.size(); j++) {

		const Triangle& triangle = triangles[j];

		// create a new temporary bbox per triangle 
		BBoxTmp b;
		b.pTri = &triangle;

		// loop over triangle vertices and pick smallest vertex for bottom of triangle bbox
		b.bottom = glm::min(b.bottom, rabbitVec3f{ vertices[triangle.indices[0]] });  // index of vertex
		b.bottom = glm::min(b.bottom, rabbitVec3f{ vertices[triangle.indices[1]] });
		b.bottom = glm::min(b.bottom, rabbitVec3f{ vertices[triangle.indices[2]] });
																				 
		// loop over triangle vertices and pick largest vertex for top of triangle bbox
		b.top = glm::max(b.top, rabbitVec3f{ vertices[triangle.indices[0]] });
		b.top = glm::max(b.top, rabbitVec3f{ vertices[triangle.indices[1]] });
		b.top = glm::max(b.top, rabbitVec3f{ vertices[triangle.indices[2]] });

		// expand working list bbox by largest and smallest triangle bbox bounds
		bottom = glm::min(bottom, b.bottom);
		top = glm::max(top, b.top);

		// compute triangle bbox center: (bbox top + bbox bottom) * 0.5
		b.center = (b.top + b.bottom) * 0.5f;

		// add triangle bbox to working list
		work.push_back(b);
	}

	// ...and pass it to the recursive function that creates the SAH AABB BVH
	// (Surface Area Heuristic, Axis-Aligned Bounding Boxes, Bounding Volume Hierarchy)

	BVHNode* root = Recurse(work); // builds BVH and returns root node

	root->bottom = bottom; // bottom is bottom of bbox bounding all triangles in the scene
	root->top = top;

	return root;
}

void CreateCFBVH(const Triangle* triangles, BVHNode* rootBVH, uint32_t** triIndexList, uint32_t* triIndexListNum, CacheFriendlyBVHNode** nodeList, uint32_t* nodeListNum)
{

	unsigned idxTriList = 0;
	unsigned idxBoxes = 0;

	*triIndexListNum = CountTriangles(rootBVH);
	*triIndexList = new uint32_t[*triIndexListNum];

	*nodeListNum = CountBoxes(rootBVH);
	*nodeList = new CacheFriendlyBVHNode[*nodeListNum]; // array

	PopulateCacheFriendlyBVH(&triangles[0], rootBVH, idxBoxes, idxTriList, *triIndexList, *nodeList);

	if ((idxBoxes != *nodeListNum - 1) || (idxTriList != *triIndexListNum)) {
		puts("Internal bug in CreateCFBVH, please report it..."); fflush(stdout);
		exit(1);
	}

	int maxDepth = 0;
	CountDepth(rootBVH, 0, maxDepth);
	//if depth is too deep do something TODO
}

int CountBoxes(BVHNode* root)
{
	if (!root->IsLeaf()) {
		BVHInner* p = dynamic_cast<BVHInner*>(root);
		return 1 + CountBoxes(p->left) + CountBoxes(p->right);
	}
	else
		return 1;
}

// recursively count triangles
unsigned CountTriangles(BVHNode* root)
{
	if (!root->IsLeaf()) {
		BVHInner* p = dynamic_cast<BVHInner*>(root);
		return CountTriangles(p->left) + CountTriangles(p->right);
	}
	else {
		BVHLeaf* p = dynamic_cast<BVHLeaf*>(root);
		return (unsigned)p->triangles.size();
	}
}

// recursively count depth
void CountDepth(BVHNode* root, int depth, int& maxDepth)
{
	if (maxDepth < depth)
		maxDepth = depth;
	if (!root->IsLeaf()) {
		BVHInner* p = dynamic_cast<BVHInner*>(root);
		CountDepth(p->left, depth + 1, maxDepth);
		CountDepth(p->right, depth + 1, maxDepth);
	}
}

// Writes in the g_pCFBVH and g_triIndexListNo arrays,
// creating a cache-friendly version of the BVH
void PopulateCacheFriendlyBVH(
	const Triangle* pFirstTriangle,
	BVHNode* root,
	unsigned& idxBoxes,
	unsigned& idxTriList,
	uint32_t* triIndexList,
	CacheFriendlyBVHNode* nodeList
	)
{
	unsigned currIdxBoxes = idxBoxes;
	nodeList[currIdxBoxes].bottom = root->bottom;
	nodeList[currIdxBoxes].top = root->top;

	//DEPTH FIRST APPROACH (left first until complete)
	if (!root->IsLeaf()) { // inner node
		BVHInner* p = dynamic_cast<BVHInner*>(root);
		// recursively populate left and right
		int idxLeft = ++idxBoxes;
		PopulateCacheFriendlyBVH(pFirstTriangle, p->left, idxBoxes, idxTriList, triIndexList, nodeList);
		int idxRight = ++idxBoxes;
		PopulateCacheFriendlyBVH(pFirstTriangle, p->right, idxBoxes, idxTriList, triIndexList, nodeList);
		nodeList[currIdxBoxes].u.inner.idxLeft = idxLeft;
		nodeList[currIdxBoxes].u.inner.idxRight = idxRight;
		nodeList[currIdxBoxes].u.inner.isLeaf = 0;

	}

	else { // leaf
		BVHLeaf* p = dynamic_cast<BVHLeaf*>(root);
		unsigned count = (unsigned)p->triangles.size();
		nodeList[currIdxBoxes].u.leaf.isLeaf = 1;
		nodeList[currIdxBoxes].u.leaf.count = count;
		nodeList[currIdxBoxes].u.leaf.startIndexInTriIndexList = idxTriList;

		for (std::list<const Triangle*>::iterator it = p->triangles.begin(); it != p->triangles.end(); it++)
		{
			triIndexList[idxTriList++] = *it - pFirstTriangle;
		}
	}
}