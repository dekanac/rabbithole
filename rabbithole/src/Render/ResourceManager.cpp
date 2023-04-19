#include "ResourceManager.h"

#include "Render/Vulkan/VulkanTexture.h"
#include "Render/Vulkan/VulkanBuffer.h"
#include "Logger/Logger.h"
#include "Utils/utils.h"

bool operator<(const AllocatedResource& lhs, const AllocatedResource& rhs)
{
	return lhs.GetID() < rhs.GetID();
}

bool operator==(const AllocatedResource& lhs, const AllocatedResource& rhs)
{
	return lhs.GetID() == rhs.GetID();
}

ResourceManager::ResourceManager()
{

}

ResourceManager::~ResourceManager()
{
	for (auto allocatedResource : m_AllocatedResources) { delete(allocatedResource); }

	m_Textures.clear();
	m_Shaders.clear();
	m_Buffers.clear();
}

VulkanTexture* ResourceManager::CreateSingleMipFromTexture(VulkanDevice& device, const VulkanTexture* texture, uint32_t mipSlice)
{
	VulkanTexture* newTextureMip = new VulkanTexture(device, texture, mipSlice);

	m_Textures[newTextureMip->GetID()] = newTextureMip;

	m_AllocatedResources.insert(newTextureMip->GetResource());
	m_AllocatedResources.insert(newTextureMip->GetSampler());
	m_AllocatedResources.insert(newTextureMip->GetView());

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
		texData = TextureLoading::LoadTextureFromFile(path);
	}

	VulkanTexture* newTexture = new VulkanTexture(device, texData, createInfo);

	TextureLoading::FreeTexture(texData);

	m_Textures[newTexture->GetID()] = newTexture;
	m_AllocatedResources.insert(newTexture->GetResource());
	m_AllocatedResources.insert(newTexture->GetSampler());
	m_AllocatedResources.insert(newTexture->GetView());

	return newTexture;
}

VulkanTexture* ResourceManager::CreateTexture(VulkanDevice& device, RWTextureCreateInfo createInfo)
{
	VulkanTexture* newTexture = new VulkanTexture(device, createInfo);

	m_Textures[newTexture->GetID()] = newTexture;
	m_AllocatedResources.insert(newTexture->GetResource());
	m_AllocatedResources.insert(newTexture->GetSampler());
	m_AllocatedResources.insert(newTexture->GetView());

	return newTexture;
}

VulkanTexture* ResourceManager::CreateTexture(VulkanDevice& device, const TextureData* data, ROTextureCreateInfo createInfo)
{
	VulkanTexture* newTexture = new VulkanTexture(device, data, createInfo);

	m_Textures[newTexture->GetID()] = newTexture;
	m_AllocatedResources.insert(newTexture->GetResource());
	m_AllocatedResources.insert(newTexture->GetSampler());
	m_AllocatedResources.insert(newTexture->GetView());

	return newTexture;
}

VulkanBuffer* ResourceManager::CreateBuffer(VulkanDevice& device, BufferCreateInfo createInfo)
{
	VulkanBuffer* newBuffer = new VulkanBuffer(device, createInfo);

	m_Buffers[newBuffer->GetID()] = newBuffer;
	m_AllocatedResources.insert(newBuffer);

	return newBuffer;
}

void ResourceManager::CreateShader(VulkanDevice& device, ShaderInfo& createInfo, const std::vector<char>& code, const char* name)
{
	Shader* shader = new Shader(device, code.size(), code.data(), createInfo, name);
	m_Shaders[{name}] = shader;
	m_AllocatedResources.insert(shader);
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
