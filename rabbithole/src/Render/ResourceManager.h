#pragma once

#include "Render/Resource.h"
#include "Render/Shader.h"
#include "Render/Model/TextureLoading.h"
#include "Render/Vulkan/VulkanTexture.h"
#include "Render/Vulkan/VulkanBuffer.h"

#include <set>

using TextureData = TextureLoading::TextureData;

bool operator<(const AllocatedResource& lhs, const AllocatedResource& rhs);

bool operator==(const AllocatedResource& lhs, const AllocatedResource& rhs);

class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();

	NonCopyableAndMovable(ResourceManager);
public:
	VulkanTexture*	CreateSingleMipFromTexture(VulkanDevice& device, const VulkanTexture* texture, uint32_t mipSlice);
	VulkanTexture*	CreateTexture(VulkanDevice& device, const TextureData* data, ROTextureCreateInfo createInfo);
	VulkanTexture*	CreateTexture(VulkanDevice& device, std::string path, ROTextureCreateInfo createInfo);
	VulkanTexture*	CreateTexture(VulkanDevice& device, RWTextureCreateInfo createInfo);
	VulkanBuffer*	CreateBuffer(VulkanDevice& device, BufferCreateInfo createInfo);
	void			CreateShader(VulkanDevice& device, ShaderInfo& createInfo, const std::vector<char>& code, const char* name);

	void DeleteBuffer(VulkanBuffer* buffer);

	Shader*											GetShader(const std::string& name);
	std::unordered_map<uint32_t, VulkanTexture*>&	GetTextures() { return m_Textures; }
private:
	template<typename AllocatedResourceType>
	void AddAllocatedResource(AllocatedResourceType* allocatedResource)
	{
		if constexpr (std::is_same<AllocatedResourceType, VulkanTexture>::value)
		{
			VulkanTexture* vulkanTexture = static_cast<VulkanTexture*>(allocatedResource);
			m_AllocatedResources.insert(vulkanTexture->GetSampler());
			m_AllocatedResources.insert(vulkanTexture->GetResource());
			m_AllocatedResources.insert(vulkanTexture->GetView());
			m_Textures[vulkanTexture->GetID()] = vulkanTexture;
		}
		else if constexpr (std::is_same<AllocatedResourceType, VulkanBuffer>::value)
		{
			VulkanBuffer* vulkanBuffer = static_cast<VulkanBuffer*>(allocatedResource);
			m_AllocatedResources.insert(vulkanBuffer);
			m_Buffers[vulkanBuffer->GetID()] = vulkanBuffer;
		}
		else if constexpr (std::is_same<AllocatedResourceType, Shader>::value)
		{
			Shader* shader = static_cast<Shader*>(allocatedResource);
			m_AllocatedResources.insert(shader);
		}
	}

	std::unordered_map<std::string, Shader*>		m_Shaders;
	std::unordered_map<uint32_t, VulkanTexture*>	m_Textures;
	std::unordered_map<uint32_t, VulkanBuffer*>		m_Buffers;
	std::set<AllocatedResource*>					m_AllocatedResources;
};