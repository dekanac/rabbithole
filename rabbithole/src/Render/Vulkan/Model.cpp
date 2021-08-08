#include "precomp.h"

#include <stddef.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader/tiny_obj_loader.h"
#include "stb_image/stb_image.h"

RabbitModel::RabbitModel(VulkanDevice& device, std::string filepath, std::string name) 
	: m_VulkanDevice{ device }
	, m_FilePath(filepath)
	, m_Name(name)
{
	LoadFromFile();
	//CreateTextures();
	CreateVertexBuffers();
	CreateIndexBuffers();
}

RabbitModel::RabbitModel(VulkanDevice& device, ModelLoading::ObjectData* objectData)
: m_VulkanDevice{ device }
{
	m_Vertices.resize(objectData->mesh->numVertices);
	m_Indices.resize(objectData->mesh->numIndices);
	m_VertexCount = objectData->mesh->numVertices;
	std::memcpy(m_Vertices.data(), objectData->mesh->pVertices, objectData->mesh->numVertices * sizeof(ModelLoading::MeshVertex));
	m_IndexCount = objectData->mesh->numIndices;
	std::memcpy(m_Indices.data(), objectData->mesh->pIndices, objectData->mesh->numIndices * sizeof(uint32_t));
	
	CreateTextures(&objectData->material);
	CreateVertexBuffers();
	CreateIndexBuffers();
 }

RabbitModel::~RabbitModel()
{
	delete m_VertexBuffer;
	if (hasIndexBuffer && m_IndexBuffer != nullptr) 
	{
		delete m_IndexBuffer;
	}
}

void RabbitModel::CreateTextures(ModelLoading::MaterialData* material)
{
	int texWidth, texHeight, texChannels;
	unsigned char* pixels = nullptr;
	if (material->diffuseMap != NULL && material->diffuseMap->pData != NULL)
	{
		texWidth = material->diffuseMap->width;
		texHeight = material->diffuseMap->height;
		texChannels = material->diffuseMap->bpp;
		pixels = material->diffuseMap->pData;
	}
	else
	{
		pixels = stbi_load("res/meshes/viking_room/viking_room.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	}
	if (!pixels) 
	{
		LOG_ERROR("failed to load texture image!");
	}
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	TextureData texData{};
	texData.bpp = texChannels;
	texData.height = texHeight;
	texData.width = texWidth;
	texData.pData = pixels;

	m_Texture = new VulkanTexture(&m_VulkanDevice, &texData, TextureFlags::Color | TextureFlags::Read, Format::R8G8B8A8_UNORM_SRGB, "box_tex");

}

void RabbitModel::CreateVertexBuffers()
{
	m_VertexCount = static_cast<uint32_t>(m_Vertices.size());
	ASSERT(m_VertexCount >= 3, "Vertex count must be greater then 3!");

	uint64_t bufferSize = sizeof(m_Vertices[0]) * m_VertexCount;

	VulkanBuffer* stagingBuffer = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::TransferSrc, MemoryAccess::Host, bufferSize);

	stagingBuffer->Map();
	memcpy(stagingBuffer->GetHostVisibleData(), m_Vertices.data(), static_cast<size_t>(bufferSize));
	stagingBuffer->Unmap();

	m_VertexBuffer = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::VertexBuffer | BufferUsageFlags::TransferDst, MemoryAccess::Device, bufferSize);

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

	VulkanBuffer* stagingBuffer = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::TransferSrc, MemoryAccess::Host, bufferSize);

	stagingBuffer->Map();
	memcpy(stagingBuffer->GetHostVisibleData(), m_Indices.data(), static_cast<size_t>(bufferSize));
	stagingBuffer->Unmap();

	m_IndexBuffer = new VulkanBuffer(&m_VulkanDevice, BufferUsageFlags::IndexBuffer | BufferUsageFlags::TransferDst, MemoryAccess::Device, bufferSize);

	m_VulkanDevice.CopyBuffer(stagingBuffer->GetBuffer(), m_IndexBuffer->GetBuffer(), bufferSize);

	delete stagingBuffer;
}

void RabbitModel::Draw(VkCommandBuffer commandBuffer)
{
	if (hasIndexBuffer)
	{
		vkCmdDrawIndexed(commandBuffer, m_IndexCount, 1, 0, 0, 0);
	}
	else
	{
		vkCmdDraw(commandBuffer, m_VertexCount, 1, 0, 0);
	}
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
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);
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
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, uv);

	return attributeDescriptions;
}
