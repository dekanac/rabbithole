#pragma once

class VulkanImage;
class VulkanImageView;
class VulkanImageSampler;
struct ModelLoading::TextureData;

class VulkanTexture
{
public:
	VulkanTexture(VulkanDevice* device, const uint32_t width, const uint32_t height, TextureFlags flags, Format format, const char* name);
	VulkanTexture(VulkanDevice* device, std::string filePath, TextureFlags flags, Format format, std::string name);
	VulkanTexture(VulkanDevice* device, TextureData* texData, TextureFlags flags, Format format, std::string name);
	~VulkanTexture();

public:
	VulkanImage*			GetResource() const { return m_Resource; }
	VulkanImageView*		GetView() const { return m_View; }
	VulkanImageSampler*		GetSampler() const { return m_Sampler; }
	Format					GetFormat() const{ return m_Format; }
	TextureFlags			GeFlags() const { return m_Flags; }
	ImageRegion				GetRegion() const { return m_Region; }

	uint32_t				GetWidth() const { return m_Region.Extent.Width; }
	uint32_t				GetHeight() const { return m_Region.Extent.Height; }

private:
	void CreateResource(VulkanDevice* device);
	void CreateResource(VulkanDevice* device, TextureData* texData);
	void CreateResource(VulkanDevice* device, const uint32_t width, const uint32_t height);
	void CreateView(VulkanDevice* device);
	void CreateSampler(VulkanDevice* device);
	void InitializeRegion(const uint32_t width, const uint32_t height);

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
};