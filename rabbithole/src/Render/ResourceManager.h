#pragma once

#include "Render/Resource.h"
#include "Render/Shader.h"

class ResourceManager
{
public:
	VulkanTexture*	CreateTexture(VulkanDevice& device, ROTextureCreateInfo createInfo);
	VulkanTexture*	CreateTexture(VulkanDevice& device, RWTextureCreateInfo createInfo);
	VulkanBuffer*	CreateBuffer(VulkanDevice& device, BufferCreateInfo createInfo);
	void			CreateShader(VulkanDevice& device, ShaderInfo& createInfo, const std::vector<char>& code, const char* name);

	inline Shader*									GetShader(const std::string& name) { return m_Shaders[name]; }
	std::unordered_map<uint32_t, VulkanTexture*>&	GetTextures() { return m_Textures; }
private:

	std::unordered_map<std::string, Shader*>		m_Shaders;
	std::unordered_map<uint32_t, VulkanTexture*>	m_Textures;
	std::unordered_map<uint32_t, VulkanBuffer*>		m_Buffers;
};