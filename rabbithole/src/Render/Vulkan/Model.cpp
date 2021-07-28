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
	CreateTextures();
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

void RabbitModel::CreateTextures()
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("res/meshes/box/box.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	VulkanBufferInfo stagingBufferInfo{};
	stagingBufferInfo.usageFlags = BufferUsageFlags::TransferSrc;
	stagingBufferInfo.memoryAccess = MemoryAccess::Host;
	stagingBufferInfo.size = imageSize;

	VulkanBuffer stagingBuffer(&m_VulkanDevice, stagingBufferInfo);

	stagingBuffer.Map();
	memcpy(stagingBuffer.GetHostVisibleData(), pixels, static_cast<size_t>(imageSize));
	stagingBuffer.Unmap();

	stbi_image_free(pixels);

	VulkanImageInfo imageInfo{};
	imageInfo.Extent.Width = texWidth;
	imageInfo.Extent.Height = texHeight;
	imageInfo.Extent.Depth = 1;
	imageInfo.Format = Format::R8G8B8A8_UNORM_SRGB;
	imageInfo.Flags = ImageFlags::LinearTiling;
	imageInfo.UsageFlags = ImageUsageFlags::TransferDst | ImageUsageFlags::Resource;
	imageInfo.MemoryAccess = MemoryAccess::Device;
	imageInfo.ArraySize = 1;
	imageInfo.MipLevels = 1;

	image = new VulkanImage(&m_VulkanDevice, imageInfo, "drametina");

	m_VulkanDevice.TransitionImageLayout(image->GetImage(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	m_VulkanDevice.CopyBufferToImage(stagingBuffer.GetBuffer(), image->GetImage(), static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1);
	m_VulkanDevice.TransitionImageLayout(image->GetImage(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VulkanImageViewInfo imageViewInfo{};
	imageViewInfo.Resource = image;
	imageViewInfo.Format = Format::R8G8B8A8_UNORM_SRGB;
	imageViewInfo.Subresource.ArraySize = 1;
	imageViewInfo.Subresource.ArraySlice = 0;
	imageViewInfo.Subresource.MipSize = 1;
	imageViewInfo.Subresource.MipSlice - 0;

	imageView = new VulkanImageView(&m_VulkanDevice, imageViewInfo, "drametina");

	VulkanImageSamplerInfo imageSamplerInfo{};
	imageSamplerInfo.AddressModeU = AddressMode::Repeat;
	imageSamplerInfo.AddressModeV = AddressMode::Repeat;
	imageSamplerInfo.AddressModeW = AddressMode::Repeat;
	imageSamplerInfo.MagFilterType = FilterType::Linear;
	imageSamplerInfo.MinFilterType = FilterType::Linear;
	imageSamplerInfo.MaxLevelOfAnisotropy = 1; //TODO: replace this with real one
	Color color{};					//TODO: do this better pls		
	color.value[0] = 0.f;
	color.value[1] = 0.f;
	color.value[2] = 0.f;
	color.value[3] = 0.f;
	imageSamplerInfo.BorderColor = color;
	imageSamplerInfo.CompareOperation = CompareOperation::Always;
	imageSamplerInfo.MipFilterType = FilterType::Linear;

	imageSampler = new VulkanImageSampler(&m_VulkanDevice, imageSamplerInfo, "drametina");

}

void RabbitModel::CreateVertexBuffers()
{
	m_VertexCount = static_cast<uint32_t>(m_Vertices.size());
	ASSERT(m_VertexCount >= 3, "Vertex count must be greater then 3!");

	VkDeviceSize bufferSize = sizeof(m_Vertices[0]) * m_VertexCount;
	
	VulkanBufferInfo stagingbufferinfo{};

	stagingbufferinfo.memoryAccess = MemoryAccess::Host;
	stagingbufferinfo.size = bufferSize;
	stagingbufferinfo.usageFlags = BufferUsageFlags::TransferSrc;

	VulkanBuffer* stagingBuffer = new VulkanBuffer(&m_VulkanDevice, stagingbufferinfo);

	stagingBuffer->Map();
	memcpy(stagingBuffer->GetHostVisibleData(), m_Vertices.data(), static_cast<size_t>(bufferSize));
	stagingBuffer->Unmap();

	VulkanBufferInfo vertexbufferinfo{};

	vertexbufferinfo.memoryAccess = MemoryAccess::Device;
	vertexbufferinfo.size = bufferSize;
	vertexbufferinfo.usageFlags = BufferUsageFlags::VertexBuffer | BufferUsageFlags::TransferDst;
	
	m_VertexBuffer = new VulkanBuffer(&m_VulkanDevice, vertexbufferinfo);

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

	VulkanBufferInfo stagingbufferinfo{};

	stagingbufferinfo.memoryAccess = MemoryAccess::Host;
	stagingbufferinfo.size = bufferSize;
	stagingbufferinfo.usageFlags = BufferUsageFlags::TransferSrc;

	VulkanBuffer* stagingBuffer = new VulkanBuffer(&m_VulkanDevice, stagingbufferinfo);

	stagingBuffer->Map();
	memcpy(stagingBuffer->GetHostVisibleData(), m_Indices.data(), static_cast<size_t>(bufferSize));
	stagingBuffer->Unmap();

	VulkanBufferInfo indexbufferinfo{};

	indexbufferinfo.memoryAccess = MemoryAccess::Device;
	indexbufferinfo.size = bufferSize;
	indexbufferinfo.usageFlags = BufferUsageFlags::IndexBuffer | BufferUsageFlags::TransferDst;

	m_IndexBuffer = new VulkanBuffer(&m_VulkanDevice, indexbufferinfo);

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
