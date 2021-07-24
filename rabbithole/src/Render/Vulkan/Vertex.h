#pragma once

#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <vector>

struct SimplePushConstantData
{
	rabbitMat4f viewMatrix;
	rabbitMat4f modelMatrix;
	rabbitMat4f projectionMatrix;
	rabbitVec3f offset;
};

class RabbitModel
{
public:

	struct Vertex
	{
		rabbitVec3f position;
		rabbitVec3f color;

		static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
		static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
	};

	struct Builder
	{
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
	};

	RabbitModel(VulkanDevice& device, const RabbitModel::Builder& builder);
	~RabbitModel();

	RabbitModel(const RabbitModel&) = delete;
	RabbitModel& operator=(const RabbitModel&) = delete;

	void Bind(VkCommandBuffer commandBuffer);
	void Draw(VkCommandBuffer commandBuffer);

private:
	void CreateVertexBuffers(const std::vector<Vertex>& vertices);
	void CreateIndexBuffers(const std::vector<uint32_t>& indices);

	VulkanDevice& m_VulkanDevice;
	VulkanBuffer* m_VertexBuffer;
	uint32_t m_VertexCount;

	bool hasIndexBuffer = false;
	VulkanBuffer* m_IndexBuffer;
	uint32_t m_IndexCount;
};
