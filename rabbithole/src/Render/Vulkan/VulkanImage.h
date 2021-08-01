#pragma once

struct ImageRegion
{
	ImageSubresourceRange Subresource;
	Offset3D Offset;
	Extent3D Extent;

};

struct VulkanImageInfo
{
	ImageFlags		Flags;
	ImageUsageFlags UsageFlags;
	MemoryAccess	MemoryAccess;
	Format			Format;
	Extent3D		Extent;
	uint16_t		ArraySize;
	uint16_t		MipLevels;
	MultisampleType MultisampleType;
};

class VulkanImage
{
public:
	VulkanImage(const VulkanDevice* device,
		const VulkanImageInfo& info,
		const char* name);
	~VulkanImage();

private:
	friend class VulkanSwapchain;
	VulkanImage(const VulkanDevice* device, const VulkanSwapchain* swapchain,
		const uint32_t backBufferIndex);

public:
	inline VulkanImageInfo GetInfo() const { return m_Info; }
	inline VkImageType	   GetImageType() const { return m_ImageType; }
	inline VkImage		   GetImage() const { return m_Image; }

private:
	const VulkanDevice* m_VulkanDevice;
	VulkanImageInfo		m_Info;
	VkFormat			m_Format;
	VkImageType			m_ImageType;
	VkImage				m_Image;
	VmaAllocation		m_Allocation;
};

struct VulkanImageViewInfo
{
	VulkanImage*	Resource;
	ImageViewFlags	Flags;
	Format			Format;
	ClearValue ClearValue;
	ImageSubresourceRange Subresource;
};

class VulkanImageView
{
public:
	VulkanImageView(const VulkanDevice* device,
		const VulkanImageViewInfo& info,
		const char* name);
	~VulkanImageView();

public:
	inline const VulkanImageViewInfo GetInfo() const { return m_Info; }
	VkImageView						 GetImageView() const { return m_ImageView; }
	VkFormat						 GetFormat() const { return m_Format; }

private:
	const VulkanDevice*			m_VulkanDevice;
	const VulkanImageViewInfo	m_Info;
	const VulkanImage*			m_Image;

	VkImageView m_ImageView;
	VkFormat	m_Format;
};

struct VulkanImageSamplerInfo
{
	AddressMode			AddressModeU;
	AddressMode			AddressModeV;
	AddressMode			AddressModeW;
	FilterType			MagFilterType;
	FilterType			MinFilterType;
	FilterType			MipFilterType;
	CompareOperation	CompareOperation;
	uint8_t				MaxLevelOfAnisotropy;
	Color				BorderColor;
	float				MipLODBias;
	float				MinLOD;
	float				MaxLOD;
};

class VulkanImageSampler
{
public:
	VulkanImageSampler(const VulkanDevice* device,
		const VulkanImageSamplerInfo& info,
		const char* name);
	~VulkanImageSampler();

public:
	inline const VulkanImageSamplerInfo GetInfo() const { return m_Info; }
	VkSampler							GetSampler() const { return m_Sampler; }
private:
	const VulkanDevice*				m_VulkanDevice;
	const VulkanImageSamplerInfo	m_Info;

	VkSampler m_Sampler;
};