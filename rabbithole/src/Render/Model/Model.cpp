#include "Render/Vulkan/precomp.h"

#include "Model.h"

#include <stddef.h>
#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader/tiny_obj_loader.h"
#include "stb_image/stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Render/Renderer.h"
#include "Render/Vulkan/VulkanTexture.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT

#include "tinygltf/tiny_gltf.h"

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
		step = (stop - start) / (1024.f / (depth + 1.f));

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
	uint32_t currIdxBoxes = idxBoxes;
	nodeList[currIdxBoxes].bottom = root->bottom;
	nodeList[currIdxBoxes].top = root->top;

	//DEPTH FIRST APPROACH (left first until complete)
	if (!root->IsLeaf()) 
	{ // inner node
		BVHInner* p = dynamic_cast<BVHInner*>(root);
		// recursively populate left and right
		int idxLeft = ++idxBoxes;
		PopulateCacheFriendlyBVH(pFirstTriangle, p->left, idxBoxes, idxTriList, triIndexList, nodeList);
		int idxRight = ++idxBoxes;
		PopulateCacheFriendlyBVH(pFirstTriangle, p->right, idxBoxes, idxTriList, triIndexList, nodeList);
		nodeList[currIdxBoxes].u.inner.idxLeft = idxLeft;
		nodeList[currIdxBoxes].u.inner.idxRight = idxRight;

	}

	else 
	{ // leaf
		BVHLeaf* p = dynamic_cast<BVHLeaf*>(root);
		uint32_t count = (unsigned)p->triangles.size();
		nodeList[currIdxBoxes].u.leaf.count = 0x80000000 | count;
		nodeList[currIdxBoxes].u.leaf.startIndexInTriIndexList = idxTriList;

		for (std::list<const Triangle*>::iterator it = p->triangles.begin(); it != p->triangles.end(); it++)
		{
			triIndexList[idxTriList++] = static_cast<uint32_t>(*it - pFirstTriangle);
		}
	}
}

void VulkanglTFModel::LoadModelFromFile(VulkanDevice* device, std::string filename)
{
	this->m_Device = device;
	tinygltf::Model glTFInput;
	tinygltf::TinyGLTF gltfContext;
	std::string error, warning;

	bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, filename);

	std::vector<uint32_t> indexBuffer;
	std::vector<Vertex> vertexBuffer;

	if (fileLoaded)
	{
		this->LoadImages(glTFInput);
		this->LoadMaterials(glTFInput);
		this->LoadTextures(glTFInput);
		const tinygltf::Scene& scene = glTFInput.scenes[0];
		for (size_t i = 0; i < scene.nodes.size(); i++)
		{
			const tinygltf::Node node = glTFInput.nodes[scene.nodes[i]];
			this->LoadNode(node, glTFInput, nullptr, indexBuffer, vertexBuffer);
		}
	}
	else
	{
		ASSERT(false, "Could not open the glTF file");
	}

	size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
	size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
	this->m_IndexCount = static_cast<uint32_t>(indexBuffer.size());

	ResourceManager* resourceManager = Renderer::instance().GetResourceManager();

	VulkanBuffer* vertexBufferGPU = resourceManager->CreateBuffer(*device, BufferCreateInfo{
			.flags = {BufferUsageFlags::VertexBuffer | BufferUsageFlags::TransferSrc},
			.memoryAccess = {MemoryAccess::GPU},
			.size = {static_cast<uint32_t>(vertexBufferSize)},
			.name = {"ModelVertexBuffer"}
		});
	vertexBufferGPU->FillBuffer(vertexBuffer.data(), vertexBufferSize);

	VulkanBuffer* indexBufferGPU = resourceManager->CreateBuffer(*device, BufferCreateInfo{
		.flags = {BufferUsageFlags::IndexBuffer | BufferUsageFlags::TransferSrc},
		.memoryAccess = {MemoryAccess::GPU},
		.size = {static_cast<uint32_t>(indexBufferSize)},
		.name = {"ModelIndexBuffer"}
		});
	indexBufferGPU->FillBuffer(indexBuffer.data(), indexBufferSize);

	this->m_VertexBuffer = vertexBufferGPU;
	this->m_IndexBuffer = indexBufferGPU;
}

VulkanglTFModel::VulkanglTFModel(VulkanDevice* device, std::string filename)
{
	LoadModelFromFile(device, filename);
}

VulkanglTFModel::~VulkanglTFModel()
{
	delete m_VertexBuffer;
	delete m_IndexBuffer;
	
	m_Textures.clear();
}

/*
	glTF loading functions

	The following functions take a glTF input model loaded via tinyglTF and convert all required data into our own structure
*/

void VulkanglTFModel::LoadImages(tinygltf::Model& input)
{
	// Images can be stored inside the glTF (which is the case for the sample model), so instead of directly
	// loading them from disk, we fetch them from the glTF loader and upload the buffers
	m_Textures.resize(input.images.size());
	for (size_t i = 0; i < input.images.size(); i++) 
	{
		tinygltf::Image& glTFImage = input.images[i];
		// Get the image data from the glTF loader
		unsigned char* buffer = nullptr;
		VkDeviceSize bufferSize = 0;
		bool deleteBuffer = false;
		// We convert RGB-only images to RGBA, as most devices don't support RGB-formats in Vulkan
		if (glTFImage.component == 3) 
		{
			bufferSize = glTFImage.width * glTFImage.height * 4;
			buffer = new unsigned char[bufferSize];
			unsigned char* rgba = buffer;
			unsigned char* rgb = &glTFImage.image[0];
			for (size_t i = 0; i < glTFImage.width * glTFImage.height; ++i) 
			{
				memcpy(rgba, rgb, sizeof(unsigned char) * 3);
				rgba += 4;
				rgb += 3;
			}
			deleteBuffer = true;
		}
		else 
		{
			buffer = &glTFImage.image[0];
			bufferSize = glTFImage.image.size();
		}

		TextureData textureData{};
		textureData.bpp = glTFImage.component;
		textureData.height = input.images[i].height;
		textureData.width = input.images[i].width;
		textureData.pData = buffer;

		VulkanTexture* texture = new VulkanTexture(m_Device, &textureData, TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst | TextureFlags::TransferSrc, Format::R8G8B8A8_UNORM, "InputTexture", true);
		m_Textures[i] = texture;

		if (deleteBuffer) 
		{
			delete buffer;
		}
	}
}

void VulkanglTFModel::LoadTextures(tinygltf::Model& input)
{
	m_TextureIndices.resize(input.textures.size());
	for (size_t i = 0; i < input.textures.size(); i++) 
	{
		m_TextureIndices[i] = input.textures[i].source;
	}
}

void VulkanglTFModel::LoadMaterials(tinygltf::Model& input)
{
	m_Materials.resize(input.materials.size());
	for (size_t i = 0; i < input.materials.size(); i++) 
	{
		// We only read the most basic properties required for our sample
		tinygltf::Material glTFMaterial = input.materials[i];
		// Get the base color factor
		if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end()) 
		{
			m_Materials[i].baseColorFactor = glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
		}
		// Get base color texture index
		if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end()) 
		{
			m_Materials[i].baseColorTextureIndex = glTFMaterial.values["baseColorTexture"].TextureIndex();
		}
		// Get metaliic and roughness texture index
		if (glTFMaterial.values.find("metallicRoughnessTexture") != glTFMaterial.values.end())
		{
			m_Materials[i].metallicRoughnessTextureIndex = glTFMaterial.values["metallicRoughnessTexture"].TextureIndex();
		}
		// Get normal texture index
		m_Materials[i].normalTextureIndex = glTFMaterial.normalTexture.index;
	}
}

void VulkanglTFModel::LoadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, VulkanglTFModel::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer)
{
	VulkanglTFModel::Node node{};
	node.matrix = glm::mat4(1.0f);

	// Get the local node matrix
	// It's either made up from translation, rotation, scale or a 4x4 matrix
	if (inputNode.translation.size() == 3) 
	{
		node.matrix = glm::translate(node.matrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
	}
	if (inputNode.rotation.size() == 4) 
	{
		glm::quat q = glm::make_quat(inputNode.rotation.data());
		node.matrix *= glm::mat4(q);
	}
	if (inputNode.scale.size() == 3) 
	{
		node.matrix = glm::scale(node.matrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
	}
	if (inputNode.matrix.size() == 16) 
	{
		node.matrix = glm::make_mat4x4(inputNode.matrix.data());
	};

	// Load node's children
	if (inputNode.children.size() > 0) 
	{
		for (size_t i = 0; i < inputNode.children.size(); i++) 
		{
			LoadNode(input.nodes[inputNode.children[i]], input, &node, indexBuffer, vertexBuffer);
		}
	}

	// If the node contains mesh data, we load vertices and indices from the buffers
	// In glTF this is done via accessors and buffer views
	if (inputNode.mesh > -1) 
	{
		const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];
		// Iterate through all primitives of this node's mesh
		for (size_t i = 0; i < mesh.primitives.size(); i++) 
		{
			const tinygltf::Primitive& glTFPrimitive = mesh.primitives[i];
			uint32_t firstIndex = static_cast<uint32_t>(indexBuffer.size());
			uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
			uint32_t indexCount = 0;
			// Vertices
			{
				const float* positionBuffer = nullptr;
				const float* normalsBuffer = nullptr;
				const float* texCoordsBuffer = nullptr;
				const float* tangentBuffer = nullptr;
				size_t vertexCount = 0;

				// Get buffer data for vertex normals
				if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end()) 
				{
					const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					positionBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					vertexCount = accessor.count;
				}
				// Get buffer data for vertex normals
				if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end()) 
				{
					const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					normalsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}
				// Get buffer data for vertex texture coordinates
				// glTF supports multiple sets, we only load the first one
				if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end()) 
				{
					const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					texCoordsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}

				if (glTFPrimitive.attributes.find("TANGENT") != glTFPrimitive.attributes.end()) 
				{
					const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TANGENT")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					tangentBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}

				// Append data to model's vertex buffer and calculate AABB
				
				AABB aabb{};
				rabbitVec3f minPos= { FLT_MAX, FLT_MAX, FLT_MAX };
				rabbitVec3f maxPos = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

				for (size_t v = 0; v < vertexCount; v++) 
				{
					Vertex vert{};
					vert.position = glm::make_vec3(&positionBuffer[v * 3]);
					vert.normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
					vert.uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec2(0.0f);
					vert.tangent = tangentBuffer ? glm::make_vec3(&tangentBuffer[v * 3]) : glm::vec3(0.0f);
					vertexBuffer.push_back(vert);

					minPos = glm::min(vert.position, minPos);
					maxPos = glm::max(vert.position, maxPos);
				}

				aabb = { minPos, maxPos };
				node.bbox = aabb;
			}
			// Indices
			{
				const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.indices];
				const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];

				indexCount += static_cast<uint32_t>(accessor.count);

				// glTF supports different component types of indices
				switch (accessor.componentType) 
				{
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: 
				{
					uint32_t* buf = new uint32_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint32_t));
					for (size_t index = 0; index < accessor.count; index++) 
					{
						indexBuffer.push_back(buf[index] + vertexStart);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: 
				{
					uint16_t* buf = new uint16_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint16_t));
					for (size_t index = 0; index < accessor.count; index++)
					{
						indexBuffer.push_back(buf[index] + vertexStart);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: 
				{
					uint8_t* buf = new uint8_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint8_t));
					for (size_t index = 0; index < accessor.count; index++)
					{
						indexBuffer.push_back(buf[index] + vertexStart);
					}
					break;
				}
				default:
					std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
					return;
				}
			}
			Primitive primitive{};
			primitive.firstIndex = firstIndex;
			primitive.indexCount = indexCount;
			primitive.materialIndex = glTFPrimitive.material;
			node.mesh.primitives.push_back(primitive);
		}
	}

	if (parent) {
		parent->children.push_back(node);
	}
	else {
		m_Nodes.push_back(node);
	}
}

void VulkanglTFModel::DrawNode(VkCommandBuffer commandBuffer, const VkPipelineLayout* pipelineLayout, VulkanglTFModel::Node node, uint8_t backBufferIndex, IndexedIndirectBuffer* indirectBuffer)
{
	if (node.mesh.primitives.size() > 0) 
	{
		// Pass the node's matrix via push constants
		// Traverse the node hierarchy to the top-most parent to get the final matrix of the current node
		glm::mat4 nodeMatrix = node.matrix;
		VulkanglTFModel::Node* currentParent = node.parent;
		while (currentParent) 
		{
			nodeMatrix = currentParent->matrix * nodeMatrix;
			currentParent = currentParent->parent;
		}
		// Pass the final matrix to the vertex shader using push constants

		for (VulkanglTFModel::Primitive& primitive : node.mesh.primitives) 
		{
			SimplePushConstantData pushData{};
			//TODO: add primitive id
			pushData.id = 1;
			pushData.modelMatrix = nodeMatrix;
			pushData.useNormalMap = m_Materials[primitive.materialIndex].normalTextureIndex != 0xFFFF;

			vkCmdPushConstants(commandBuffer, *pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &pushData);

			//TODO: decrease num of descriptor set binding to number of different materials
			//sort primitives by materialIndexNumber
			if (primitive.indexCount > 0) 
			{
				VulkanDescriptorSet* materialDescriptorSet = m_Materials[primitive.materialIndex].materialDescriptorSet[backBufferIndex];
				// Bind the descriptor for the current primitive's texture
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0, 1, materialDescriptorSet->GetVkDescriptorSet(), 0, nullptr);

				IndexIndirectDrawData indexIndirectDrawCommand{};
                indexIndirectDrawCommand.firstIndex = primitive.firstIndex;
                indexIndirectDrawCommand.firstInstance = 0;
                indexIndirectDrawCommand.indexCount = primitive.indexCount;
                indexIndirectDrawCommand.instanceCount = 1;
                indexIndirectDrawCommand.vertexOffset = 0;

				indirectBuffer->AddIndirectDrawCommand(commandBuffer, indexIndirectDrawCommand);
			}
		}
	}
	for (auto& child : node.children) 
	{
		DrawNode(commandBuffer, pipelineLayout, child, backBufferIndex, indirectBuffer);
	}
}



void VulkanglTFModel::Draw(VkCommandBuffer commandBuffer, const VkPipelineLayout* pipeLayout, uint8_t backBufferIndex, IndexedIndirectBuffer* indirectBuffer)
{
	for (auto& node : m_Nodes)
	{
		DrawNode(commandBuffer, pipeLayout, node, backBufferIndex, indirectBuffer);
	}
}

void VulkanglTFModel::BindBuffers(VkCommandBuffer commandBuffer)
{
	VkDeviceSize offsets[1] = { 0 };
	VkBuffer vertexBuffer = m_VertexBuffer->GetBuffer();
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
}