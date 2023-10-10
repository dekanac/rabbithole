#include "ResourceManager.h"

#include "Logger/Logger.h"
#include "Utils/Utils.h"

ResourceManager::ResourceManager()
{

}

ResourceManager::~ResourceManager()
{
	for (AllocatedResource* allocatedResource : m_AllocatedResources) 
	{ 
		if (allocatedResource) 
			delete(allocatedResource); 
	}

	m_Textures.clear();
	m_Shaders.clear();
	m_Buffers.clear();
}

VulkanTexture* ResourceManager::CreateSingleMipFromTexture(VulkanDevice& device, const VulkanTexture* texture, uint32_t mipSlice)
{
	VulkanTexture* newTextureMip = new VulkanTexture(device, texture, mipSlice);

	AddAllocatedResource(newTextureMip);

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
		texData->width = cubeMapData->pData[0]->width;
		texData->height = cubeMapData->pData[0]->height;
		texData->size = texData->height * texData->width * 4 * 6;
		texData->pData = RABBIT_ALLOC(unsigned char, texData->size);
		for (int i = 0; i < 6; i++)
		{
			memcpy(texData->pData + i * texData->size / 6, cubeMapData->pData[i]->pData, texData->size / 6);
		}

		TextureLoading::FreeCubemap(cubeMapData);
	}
	else
	{
		texData = TextureLoading::LoadTextureFromFile(path);
	}

	VulkanTexture* newTexture = new VulkanTexture(device, texData, createInfo);

	TextureLoading::FreeTexture(texData);

	AddAllocatedResource(newTexture);

	return newTexture;
}

VulkanTexture* ResourceManager::CreateTexture(VulkanDevice& device, RWTextureCreateInfo createInfo)
{
	VulkanTexture* newTexture = new VulkanTexture(device, createInfo);

	AddAllocatedResource(newTexture);

	return newTexture;
}

VulkanTexture* ResourceManager::CreateTexture(VulkanDevice& device, const TextureData* data, ROTextureCreateInfo createInfo)
{
	VulkanTexture* newTexture = new VulkanTexture(device, data, createInfo);

	AddAllocatedResource(newTexture);

	return newTexture;
}

VulkanBuffer* ResourceManager::CreateBuffer(VulkanDevice& device, BufferCreateInfo createInfo)
{
	VulkanBuffer* newBuffer = new VulkanBuffer(device, createInfo);

	AddAllocatedResource(newBuffer);

	return newBuffer;
}

void ResourceManager::CreateShader(VulkanDevice& device, ShaderInfo& createInfo, const char* code, size_t codeSize, const char* name)
{
	Shader* shader = new Shader(device, codeSize, code, createInfo, name);
	
	std::string shaderName(name);
	if (m_Shaders.find(shaderName) == m_Shaders.end())
	{
		m_Shaders[shaderName] = shader;
	}
	else
	{
		m_Shaders.erase(shaderName);
		m_Shaders[shaderName] = shader;
	}

	AddAllocatedResource(shader);
}

void ResourceManager::DeleteBuffer(VulkanBuffer* buffer)
{
	m_AllocatedResources.erase(buffer);
	delete buffer;
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
