#include "precomp.h"

VulkanImage::VulkanImage(const VulkanDevice* device, const VulkanImageInfo& info, const char* name)
	: m_VulkanDevice(device)
	, m_Info(info)
	, m_Format(GetVkFormatFrom(m_Info.Format))
	, m_ImageType(GetVkImageTypeFrom(m_Info.Extent.Depth))
{
	VkImageCreateInfo imageCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };

	uint32_t arraySizeMultiplier = 1;
	if (IsFlagSet(m_Info.Flags & ImageFlags::CubeMap))
	{
		arraySizeMultiplier = 6;
		imageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	uint32_t blockSize = GetBlockSizeFrom(m_Format);
	uint32_t blockAlignedWidth = m_Info.Extent.Width - m_Info.Extent.Width % blockSize + blockSize;
	uint32_t blockAlignedHeight = m_Info.Extent.Height - m_Info.Extent.Height % blockSize + blockSize;

	imageCreateInfo.imageType = m_Info.Extent.Depth == 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D;
	imageCreateInfo.format = m_Format;
	imageCreateInfo.extent.width = blockAlignedWidth;
	imageCreateInfo.extent.height = blockAlignedHeight;
	imageCreateInfo.extent.depth = m_Info.Extent.Depth;
	imageCreateInfo.mipLevels = m_Info.MipLevels;
	imageCreateInfo.arrayLayers = arraySizeMultiplier * m_Info.ArraySize;
	imageCreateInfo.samples = GetVkSampleFlagsFrom(m_Info.MultisampleType);
	imageCreateInfo.tiling = IsFlagSet(m_Info.Flags & ImageFlags::LinearTiling) ?
		VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = GetVkImageUsageFlagsFrom(m_Info.UsageFlags);
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo allocationCreateInfo = {};
	allocationCreateInfo.usage = GetVmaMemoryUsageFrom(m_Info.MemoryAccess);

	VULKAN_API_CALL(vmaCreateImage(m_VulkanDevice->GetVmaAllocator(), &imageCreateInfo, &allocationCreateInfo, &m_Image, &m_Allocation, nullptr));
}

VulkanImage::VulkanImage(const VulkanDevice* device, const VulkanSwapchain* swapchain, const uint32_t backBufferIndex)
{
	//TODO: implement this for swapchain images
}

VulkanImage::~VulkanImage()
{
	if (m_Allocation != VK_NULL_HANDLE)
	{
		vmaDestroyImage(m_VulkanDevice->GetVmaAllocator(), m_Image, m_Allocation);
	}
}

VulkanImageView::VulkanImageView(const VulkanDevice* device, const VulkanImageViewInfo& info, const char* name)
	: m_VulkanDevice(device)
	, m_Info(info)
{
	VulkanImage* image = m_Info.Resource;
	m_Format = GetVkFormatFrom(m_Info.Format == Format::UNDEFINED ? image->GetInfo().Format : m_Info.Format);

	VkImageViewCreateInfo imageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewCreateInfo.format = m_Format;

	uint32_t arraySizeMult = 1;
	if (IsFlagSet(image->GetInfo().Flags & ImageFlags::CubeMap))
	{
		arraySizeMult = 6;
		imageViewCreateInfo.viewType = image->GetInfo().ArraySize > 1 ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
	}
	else if (image->GetImageType() == VK_IMAGE_TYPE_2D)
	{
		imageViewCreateInfo.viewType = image->GetInfo().ArraySize > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
	}
	else if (image->GetImageType() == VK_IMAGE_TYPE_3D)
	{
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
	}
	else
	{
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_1D;
	}

	imageViewCreateInfo.subresourceRange.aspectMask = GetVkImageAspectFlagsFrom(m_Format);
	imageViewCreateInfo.subresourceRange.baseMipLevel = m_Info.Subresource.MipSlice;
	imageViewCreateInfo.subresourceRange.levelCount = m_Info.Subresource.MipSize;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = m_Info.Subresource.ArraySlice * arraySizeMult;
	imageViewCreateInfo.subresourceRange.layerCount = m_Info.Subresource.ArraySize * arraySizeMult;
	imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	imageViewCreateInfo.image = image->GetImage();

	VULKAN_API_CALL(vkCreateImageView(m_VulkanDevice->GetGraphicDevice(), &imageViewCreateInfo, nullptr, &m_ImageView));
}

VulkanImageView::~VulkanImageView()
{
	vkDestroyImageView(m_VulkanDevice->GetGraphicDevice(), m_ImageView, nullptr);
}

VulkanImageSampler::VulkanImageSampler(const VulkanDevice* device, const VulkanImageSamplerInfo& info, const char* name)
	: m_VulkanDevice(device)
	, m_Info(info)
{
	VkSamplerCreateInfo samplerCreateInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerCreateInfo.magFilter = GetVkFilterFrom(m_Info.MagFilterType);
	samplerCreateInfo.minFilter = GetVkFilterFrom(m_Info.MinFilterType);
	samplerCreateInfo.mipmapMode = GetVkMipmapModeFrom(m_Info.MipFilterType);
	samplerCreateInfo.addressModeU = GetVkAddressModeFrom(m_Info.AddressModeU);
	samplerCreateInfo.addressModeV = GetVkAddressModeFrom(m_Info.AddressModeV);
	samplerCreateInfo.addressModeW = GetVkAddressModeFrom(m_Info.AddressModeW);
	samplerCreateInfo.mipLodBias = m_Info.MipLODBias;
	samplerCreateInfo.compareEnable = m_Info.CompareOperation != CompareOperation::Never;
	samplerCreateInfo.compareOp = GetVkCompareOperationFrom(m_Info.CompareOperation);
	samplerCreateInfo.minLod = m_Info.MinLOD;
	samplerCreateInfo.maxLod = m_Info.MaxLOD;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	samplerCreateInfo.borderColor = GetVkBorderColorFrom(m_Info.BorderColor);
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.maxAnisotropy = m_Info.MaxLevelOfAnisotropy;

	VULKAN_API_CALL(vkCreateSampler(m_VulkanDevice->GetGraphicDevice(), &samplerCreateInfo, nullptr, &m_Sampler));
}

VulkanImageSampler::~VulkanImageSampler()
{
	vkDestroySampler(m_VulkanDevice->GetGraphicDevice(), m_Sampler, nullptr);
}
