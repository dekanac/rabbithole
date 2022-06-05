#pragma once

#include "Render/Model/TextureLoading.h"
#include "Render/Resource.h"

class VulkanImage;
class VulkanImageView;
class VulkanImageSampler;

using namespace TextureLoading;

class VulkanTexture : public ManagableResource
{
public:
	VulkanTexture(VulkanDevice* device, const uint32_t width, const uint32_t height, TextureFlags flags, Format format, const char* name);
	VulkanTexture(VulkanDevice* device, const uint32_t width, const uint32_t height, TextureFlags flags, Format format, uint32_t arraySize, const char* name);
	VulkanTexture(VulkanDevice* device, std::string filePath, TextureFlags flags, Format format, std::string name, bool generateMips = false);
	VulkanTexture(VulkanDevice* device, TextureData* texData, TextureFlags flags, Format format, std::string name, bool generateMips = false);
	~VulkanTexture();

public:
	VulkanImage*			GetResource() const { return m_Resource; }
	VulkanImageView*		GetView() const { return m_View; }
	VulkanImageSampler*		GetSampler() const { return m_Sampler; }
	Format					GetFormat() const{ return m_Format; }
	TextureFlags			GeFlags() const { return m_Flags; }
	ImageRegion				GetRegion() const { return m_Region; }

	virtual ResourceState			GetResourceState() const { return m_CurrentResourceState; };
	virtual void					SetResourceState(ResourceState state) { m_CurrentResourceState = state; }

	virtual ResourceState			GetShouldBeResourceState() const { return m_ShouldBeResourceState; }
	virtual void					SetShouldBeResourceState(ResourceState state) { m_ShouldBeResourceState = state; }

	uint32_t				GetWidth() const { return m_Region.Extent.Width; }
	uint32_t				GetHeight() const { return m_Region.Extent.Height; }

private:
	void CreateResource(VulkanDevice* device, TextureData* texData, bool generateMips = false);
	void CreateResource(VulkanDevice* device, const uint32_t width, const uint32_t height, uint32_t arraySize = 1);
	void CreateView(VulkanDevice* device);
	void CreateSampler(VulkanDevice* device);
	void InitializeRegion(const uint32_t width, const uint32_t height, uint32_t arraySize = 1, uint32_t mipCount = 1);
	void GenerateMips(VkCommandBuffer commandBuffer, VulkanDevice* device, const uint32_t width, const uint32_t height, uint32_t mipCount);

private:
	VulkanImage*			m_Resource;
	VulkanImageView*		m_View;
	VulkanImageSampler*		m_Sampler;
	ImageRegion				m_Region;

	Format					m_Format;
	TextureFlags			m_Flags;
	std::string				m_FilePath;
	std::string				m_Name;
	TextureData*			m_TexData;

	ResourceState			m_CurrentResourceState;
	ResourceState			m_ShouldBeResourceState;
};