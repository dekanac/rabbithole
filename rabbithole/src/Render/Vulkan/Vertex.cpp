#include "precomp.h"

#include <stddef.h>

RabbitModel::RabbitModel(VulkanDevice& device, const RabbitModel::Builder& builder) : m_VulkanDevice{ device }
{
	CreateVertexBuffers(builder.vertices);
	CreateIndexBuffers(builder.indices);
}

RabbitModel::~RabbitModel()
{
	delete m_VertexBuffer;
	if (hasIndexBuffer && m_IndexBuffer != nullptr) 
	{
		delete m_IndexBuffer;
	}
}

void RabbitModel::CreateVertexBuffers(const std::vector<Vertex>& vertices)
{
	m_VertexCount = static_cast<uint32_t>(vertices.size());
	ASSERT(m_VertexCount >= 3, "Vertex count must be greater then 3!");

	VkDeviceSize bufferSize = sizeof(vertices[0]) * m_VertexCount;
	
	VulkanBufferInfo stagingbufferinfo{};

	stagingbufferinfo.memoryAccess = MemoryAccess::Host;
	stagingbufferinfo.size = bufferSize;
	stagingbufferinfo.usageFlags = BufferUsageFlags::TransferSrc;

	VulkanBuffer* stagingBuffer = new VulkanBuffer(&m_VulkanDevice, stagingbufferinfo);

	stagingBuffer->Map();
	memcpy(stagingBuffer->GetHostVisibleData(), vertices.data(), static_cast<size_t>(bufferSize));
	stagingBuffer->Unmap();

	VulkanBufferInfo vertexbufferinfo{};

	vertexbufferinfo.memoryAccess = MemoryAccess::Device;
	vertexbufferinfo.size = bufferSize;
	vertexbufferinfo.usageFlags = BufferUsageFlags::VertexBuffer | BufferUsageFlags::TransferDst;
	
	m_VertexBuffer = new VulkanBuffer(&m_VulkanDevice, vertexbufferinfo);

	m_VulkanDevice.CopyBuffer(stagingBuffer->GetBuffer(), m_VertexBuffer->GetBuffer(), bufferSize);

	delete stagingBuffer;
}

void RabbitModel::CreateIndexBuffers(const std::vector<uint32_t>& indices)
{
	m_IndexCount = static_cast<uint32_t>(indices.size());
	hasIndexBuffer = m_IndexCount > 0;

	if (!hasIndexBuffer)
	{
		return;
	}

	VkDeviceSize bufferSize = sizeof(indices[0]) * m_IndexCount;

	VulkanBufferInfo stagingbufferinfo{};

	stagingbufferinfo.memoryAccess = MemoryAccess::Host;
	stagingbufferinfo.size = bufferSize;
	stagingbufferinfo.usageFlags = BufferUsageFlags::TransferSrc;

	VulkanBuffer* stagingBuffer = new VulkanBuffer(&m_VulkanDevice, stagingbufferinfo);

	stagingBuffer->Map();
	memcpy(stagingBuffer->GetHostVisibleData(), indices.data(), static_cast<size_t>(bufferSize));
	stagingBuffer->Unmap();

	VulkanBufferInfo indexbufferinfo{};

	indexbufferinfo.memoryAccess = MemoryAccess::Device;
	indexbufferinfo.size = bufferSize;
	indexbufferinfo.usageFlags = BufferUsageFlags::VertexBuffer | BufferUsageFlags::TransferDst;

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

std::vector<VkVertexInputBindingDescription> RabbitModel::Vertex::GetBindingDescriptions()
{
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> RabbitModel::Vertex::GetAttributeDescriptions()
{
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, position);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	return attributeDescriptions;
}
