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

struct SimplePushConstantData
{
	rabbitMat4f model;
	rabbitVec3f cameraPosition;
};

struct Vertex
{
	rabbitVec3f position;
	rabbitVec3f normal;
	rabbitVec2f uv;

	static std::vector<VkVertexInputBindingDescription>		GetBindingDescriptions();
	static std::vector<VkVertexInputAttributeDescription>	GetAttributeDescriptions();

	bool operator==(const Vertex& other) const
	{
		return position == other.position && normal == other.normal && uv == other.uv;
	}
};

struct TextureData
{
	unsigned char* pData;
	int width;
	int height;
	int bpp;
};

namespace std
{
	template <>
	struct hash<Vertex>
	{
		size_t operator()(const Vertex& k) const
		{
			size_t res = 17;
			res = res * 31 + hash<rabbitVec3f>()(k.position);
			res = res * 31 + hash<rabbitVec3f>()(k.normal);
			res = res * 31 + hash<rabbitVec2f>()(k.uv);
			return res;
		}
	};
}

class RabbitModel
{
public:

	RabbitModel(VulkanDevice& device, std::string filepath, std::string name);
	RabbitModel(VulkanDevice& device, ModelLoading::ObjectData* objectData);
	~RabbitModel();

	RabbitModel(const RabbitModel&) = delete;
	RabbitModel& operator=(const RabbitModel&) = delete;

	void Bind(VkCommandBuffer commandBuffer);
	void Draw(VkCommandBuffer commandBuffer);

	void LoadFromFile();

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

	TextureData*			m_TextureData{};

public:	//TODO: clean this up, this is for test only
	VulkanImage*			image;
	VulkanImageView*		imageView;
	VulkanImageSampler*		imageSampler;
private:
	std::string				m_FilePath{};
	std::string				m_Name{};
};
