#include "ResourceManager.h"

#include "Render/Vulkan/VulkanTexture.h"
#include "Render/Vulkan/VulkanBuffer.h"
#include "Logger/Logger.h"
#include "Utils/utils.h"

ResourceManager::~ResourceManager()
{
	//TODO: since VulkanTexture can have reference to some other's texture Resource and Sampler
	// this should be reimplemented not to delete shared resources
	for (auto texture : m_Textures) { delete(texture.second); }
	for (auto shader : m_Shaders) { delete(shader.second); }
	for (auto buffer : m_Buffers) { delete(buffer.second); }

	m_Textures.clear();
	m_Shaders.clear();
	m_Buffers.clear();
}

VulkanTexture* ResourceManager::CreateSingleMipFromTexture(VulkanDevice& device, const VulkanTexture* texture, uint32_t mipSlice)
{
	VulkanTexture* newTextureMip = new VulkanTexture(device, texture, mipSlice);

	m_Textures[newTextureMip->GetID()] = newTextureMip;

	return newTextureMip;
}

VulkanTexture* ResourceManager::CreateTexture(VulkanDevice& device, std::string path, ROTextureCreateInfo createInfo)
{
	bool isCubeMap = IsFlagSet(createInfo.flags & TextureFlags::CubeMap);

	TextureData* texData = nullptr;

	if (isCubeMap)
	{
		auto cubeMapData = TextureLoading::LoadCubemap(path);

		texData = new TextureData{};
		texData->bpp = cubeMapData->pData[0]->bpp;
		texData->width = cubeMapData->pData[0]->width;
		texData->height = cubeMapData->pData[0]->height;

		size_t imageSize = texData->height * texData->width * 4;

		texData->pData = RABBIT_ALLOC(unsigned char, imageSize * 6);
		for (int i = 0; i < 6; i++)
		{
			memcpy(texData->pData + i * imageSize, cubeMapData->pData[i]->pData, imageSize);
		}

		TextureLoading::FreeCubemap(cubeMapData);
	}
	else
	{
		texData = TextureLoading::LoadTexture(path);
	}

	VulkanTexture* newTexture = new VulkanTexture(device, texData, createInfo);

	TextureLoading::FreeTexture(texData);

	m_Textures[newTexture->GetID()] = newTexture;

	return newTexture;
}

VulkanTexture* ResourceManager::CreateTexture(VulkanDevice& device, RWTextureCreateInfo createInfo)
{
	VulkanTexture* newTexture = new VulkanTexture(device, createInfo);

	m_Textures[newTexture->GetID()] = newTexture;

	return newTexture;
}

VulkanTexture* ResourceManager::CreateTexture(VulkanDevice& device, const TextureData* data, ROTextureCreateInfo createInfo)
{
	VulkanTexture* newTexture = new VulkanTexture(device, data, createInfo);

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

Shader* ResourceManager::GetShader(const std::string& name)
{
	auto shader = m_Shaders.find(name);
	if (shader != m_Shaders.end())
	{
		return shader->second;
	}
	else
	{
		ASSERT(false, "Cannot find shader with that name!");
		return nullptr;
	}
}
