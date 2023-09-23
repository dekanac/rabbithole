#include "precomp.h"

#include "Render/Converters.h"

VulkanImage::VulkanImage(const VulkanDevice* device, const VulkanImageInfo& info, const char* name)
	: m_VulkanDevice(device)
	, m_Info(info)
	, m_Format(GetVkFormatFrom(m_Info.Format))
	, m_ImageType(GetVkImageTypeFrom(m_Info.Extent.Depth))
{
	VkImageCreateInfo imageCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };

	imageCreateInfo.flags |= IsFlagSet(m_Info.Flags & ImageFlags::CubeMap) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	imageCreateInfo.imageType = m_Info.Extent.Depth == 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D;
	imageCreateInfo.format = m_Format;
	imageCreateInfo.extent.width = m_Info.Extent.Width;
	imageCreateInfo.extent.height = m_Info.Extent.Height;
	imageCreateInfo.extent.depth = m_Info.Extent.Depth;
	imageCreateInfo.mipLevels = m_Info.MipLevels;
	imageCreateInfo.arrayLayers = m_Info.ArraySize;
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
	m_Info.Flags = ImageFlags::None;
	m_Info.UsageFlags =
		ImageUsageFlags::TransferDst |
		ImageUsageFlags::Resource |
		ImageUsageFlags::Storage |
		ImageUsageFlags::RenderTarget;
	m_Info.MemoryAccess = MemoryAccess::GPU;
	m_Info.Format = Format::B8G8R8A8_UNORM_SRGB; //swapchain->GetSwapChainImageFormat(); //TODO: add proper fields to swapchain class
	m_Info.Extent.Width = swapchain->GetSwapChainExtent().width;
	m_Info.Extent.Height = swapchain->GetSwapChainExtent().height;
	m_Info.Extent.Depth = 1;
	m_Info.ArraySize = 1;
	m_Info.MipLevels = 1;
	m_Info.MultisampleType = MultisampleType::Sample_1;

	m_VulkanDevice = nullptr;
	m_Image = swapchain->m_SwapChainImages[backBufferIndex];
	m_Allocation = VK_NULL_HANDLE;
	m_Format = GetVkFormatFrom(m_Info.Format);
	m_ImageType = VK_IMAGE_TYPE_2D;
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
	VkImageViewCreateInfo imageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewCreateInfo.format = GetVkFormatFrom(m_Info.Format);

	if (IsFlagSet(info.Resource->GetInfo().Flags & ImageFlags::CubeMap))
	{
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	}
	else if (info.Resource->GetImageType() == VK_IMAGE_TYPE_2D)
	{
		imageViewCreateInfo.viewType = info.Resource->GetInfo().ArraySize > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
	}
	else if (info.Resource->GetImageType() == VK_IMAGE_TYPE_3D)
	{
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
	}
	else
	{
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_1D;
	}

	imageViewCreateInfo.subresourceRange.aspectMask = GetVkImageAspectFlagsFrom(GetVkFormatFrom(m_Info.Format));
	imageViewCreateInfo.subresourceRange.baseMipLevel = m_Info.Subresource.MipSlice;
	imageViewCreateInfo.subresourceRange.levelCount = m_Info.Subresource.MipSize;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = m_Info.Subresource.ArraySlice;
	imageViewCreateInfo.subresourceRange.layerCount = m_Info.Subresource.ArraySize;
	imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	imageViewCreateInfo.image = GET_VK_HANDLE_PTR(info.Resource);

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
	samplerCreateInfo.anisotropyEnable = m_Info.MaxLevelOfAnisotropy != 0 ? VK_TRUE : VK_FALSE;
	samplerCreateInfo.maxAnisotropy = m_Info.MaxLevelOfAnisotropy;

	VULKAN_API_CALL(vkCreateSampler(m_VulkanDevice->GetGraphicDevice(), &samplerCreateInfo, nullptr, &m_Sampler));
}

VulkanImageSampler::~VulkanImageSampler()
{
	vkDestroySampler(m_VulkanDevice->GetGraphicDevice(), m_Sampler, nullptr);
}
