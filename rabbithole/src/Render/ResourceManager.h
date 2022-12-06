#pragma once

#include "Render/Resource.h"
#include "Render/Shader.h"
#include "Render/Model/TextureLoading.h"

using TextureData = TextureLoading::TextureData;

class ResourceManager
{
public:
	~ResourceManager();

public:
	VulkanTexture*	CreateSingleMipFromTexture(VulkanDevice& device, const VulkanTexture* texture, uint32_t mipSlice);
	VulkanTexture*	CreateTexture(VulkanDevice& device, const TextureData* data, ROTextureCreateInfo createInfo);
	VulkanTexture*	CreateTexture(VulkanDevice& device, std::string path, ROTextureCreateInfo createInfo);
	VulkanTexture*	CreateTexture(VulkanDevice& device, RWTextureCreateInfo createInfo);
	VulkanBuffer*	CreateBuffer(VulkanDevice& device, BufferCreateInfo createInfo);
	void			CreateShader(VulkanDevice& device, ShaderInfo& createInfo, const std::vector<char>& code, const char* name);

	Shader*											GetShader(const std::string& name);
	std::unordered_map<uint32_t, VulkanTexture*>&	GetTextures() { return m_Textures; }
private:

	std::unordered_map<std::string, Shader*>		m_Shaders;
	std::unordered_map<uint32_t, VulkanTexture*>	m_Textures;
	std::unordered_map<uint32_t, VulkanBuffer*>		m_Buffers;
};