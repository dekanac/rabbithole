#include "ResourceManager.h"

#include "Render/Vulkan/VulkanTexture.h"
#include "Render/Vulkan/VulkanBuffer.h"

VulkanTexture* ResourceManager::CreateTexture(VulkanDevice& device, ROTextureCreateInfo createInfo)
{
	VulkanTexture* newTexture = new VulkanTexture(device, createInfo);
	
	m_Textures[newTexture->GetID()] = newTexture;

	return newTexture;
}

VulkanTexture* ResourceManager::CreateTexture(VulkanDevice& device, RWTextureCreateInfo createInfo)
{
	VulkanTexture* newTexture = new VulkanTexture(device, createInfo);

	m_Textures[newTexture->GetID()] = newTexture;

	return newTexture;
}

VulkanBuffer* ResourceManager::CreateBuffer(VulkanDevice& device, BufferCreateInfo createInfo)
{
	VulkanBuffer* newBuffer = new VulkanBuffer(device, createInfo);

	m_Buffers[newBuffer->GetID()] = newBuffer;

	return newBuffer;
}

void ResourceManager::CreateShader(VulkanDevice& device, ShaderInfo& createInfo, const std::vector<char>& code, const char* name)
{
	Shader* shader = new Shader(device, code.size(), code.data(), createInfo, name);
	m_Shaders[{name}] = shader;
}
