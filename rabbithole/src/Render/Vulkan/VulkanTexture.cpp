#include "precomp.h"

#include "Render/Model/Model.h"
#include "Render/Model/TextureLoading.h"

VulkanTexture::VulkanTexture(VulkanDevice* device, std::string filePath, TextureFlags flags, Format format, std::string name, bool generateMips)
	: m_Format(format)
	, m_Flags(flags)
	, m_FilePath(filePath)
	, m_Name(name)
{
	auto texData = TextureLoading::LoadTexture(filePath);

	CreateResource(device, texData, generateMips);
	CreateView(device);
	CreateSampler(device);

	TextureLoading::FreeTexture(texData);
}

VulkanTexture::VulkanTexture(VulkanDevice* device, const uint32_t width, const uint32_t height, TextureFlags flags, Format format, const char* name, uint32_t arraySize, MultisampleType mstype)
	: m_Format(format)
	, m_Flags(flags)
	, m_FilePath("")
	, m_Name(name)
{
	CreateResource(device, width, height, arraySize, mstype);
	CreateView(device);
	CreateSampler(device);

	device->SetObjectName((uint64_t)(m_Resource->GetImage()), VK_OBJECT_TYPE_IMAGE, name);
	device->SetObjectName((uint64_t)m_View->GetImageView(), VK_OBJECT_TYPE_IMAGE_VIEW, name);
	device->SetObjectName((uint64_t)m_Sampler->GetSampler(), VK_OBJECT_TYPE_SAMPLER, name);
}

VulkanTexture::VulkanTexture(VulkanDevice* device, TextureData* texData, TextureFlags flags, Format format, std::string name, bool generateMips)
	: m_Format(format)
	, m_Flags(flags)
	, m_FilePath("")
	, m_Name(name)
{
	CreateResource(device, texData, generateMips);
	CreateView(device);
	CreateSampler(device);
}

VulkanTexture::~VulkanTexture()
{
	delete(m_Sampler);
	delete(m_View);
	delete(m_Resource);
}

void VulkanTexture::CreateResource(VulkanDevice* device, TextureData* texData, bool generateMips)
{
	bool isCubeMap = IsFlagSet(m_Flags & TextureFlags::CubeMap);

	uint32_t mipCount = generateMips ? (static_cast<uint32_t>(std::floor(std::log2(std::max(texData->width, texData->height)))) + 1) : 1;
	uint32_t arraySize = 1 * (isCubeMap ? 6 : 1);

	InitializeRegion(texData->width, texData->height, arraySize, mipCount);

	int textureSize = texData->height * texData->width * GetBPPFrom(m_Format) * arraySize;

	VulkanBufferInfo bufferInfo{};
	bufferInfo.memoryAccess = MemoryAccess::CPU;
	bufferInfo.usageFlags = BufferUsageFlags::StorageBuffer | BufferUsageFlags::TransferSrc;
	bufferInfo.size = textureSize;

	VulkanBuffer stagingBuffer(device, bufferInfo, "StagingBuffer");

	void* data = stagingBuffer.Map();
	memcpy(data, texData->pData, textureSize);
	stagingBuffer.Unmap();

	VulkanImageInfo textureResourceInfo;
	textureResourceInfo.Flags = (isCubeMap ? ImageFlags::CubeMap : ImageFlags::None) |
								(IsFlagSet(m_Flags & TextureFlags::LinearTiling) ? ImageFlags::LinearTiling : ImageFlags::None);
	textureResourceInfo.UsageFlags = ImageUsageFlags::Resource |
		(IsFlagSet(m_Flags & TextureFlags::TransferDst) ? ImageUsageFlags::TransferDst : ImageUsageFlags::None) | 
		(IsFlagSet(m_Flags & TextureFlags::TransferSrc) ? ImageUsageFlags::TransferSrc : ImageUsageFlags::None) | 
		(IsFlagSet(m_Flags & TextureFlags::DepthStencil) ? ImageUsageFlags::DepthStencil : ImageUsageFlags::None) | 
		(IsFlagSet(m_Flags & TextureFlags::RenderTarget) ? ImageUsageFlags::RenderTarget : ImageUsageFlags::None);
	textureResourceInfo.MemoryAccess = MemoryAccess::GPU;
	textureResourceInfo.Format = m_Format;
	textureResourceInfo.Extent.Width = texData->width;
	textureResourceInfo.Extent.Height = texData->height;
	textureResourceInfo.Extent.Depth = 1;
	textureResourceInfo.ArraySize = arraySize;
	textureResourceInfo.MipLevels = mipCount;
	textureResourceInfo.MultisampleType = MultisampleType::Sample_1;

	m_Resource = new VulkanImage(device, textureResourceInfo, m_Name.c_str());

	ResourceState stateAfter = ResourceState::None;
	if (IsFlagSet(m_Flags & TextureFlags::Read) && IsFlagSet(m_Flags & TextureFlags::DepthStencil))
	{
		stateAfter = ResourceState::DepthStencilRead;
	}
	else if (IsFlagSet(m_Flags & TextureFlags::Read))
	{
		stateAfter = ResourceState::GenericRead;
	}
	else if (IsFlagSet(m_Flags & TextureFlags::RenderTarget) && IsFlagSet(m_Flags & TextureFlags::DepthStencil))
	{
		stateAfter = ResourceState::DepthStencilWrite;
	}
	else if (IsFlagSet(m_Flags & TextureFlags::RenderTarget))
	{
		stateAfter = ResourceState::RenderTarget;
	}

	VkCommandBuffer commandBuffer = device->BeginSingleTimeCommands();

	device->TransitionImageLayout(commandBuffer, this, ResourceState::None, ResourceState::TransferDst, 0, mipCount);

	if (isCubeMap)
	{
		device->CopyBufferToImageCubeMap(commandBuffer, &stagingBuffer, this);
	}
	else
	{
		device->CopyBufferToImage(commandBuffer, &stagingBuffer, this, true);
	}

	m_ShouldBeResourceState = m_CurrentResourceState = stateAfter;

	if (generateMips)
	{
		GenerateMips(commandBuffer, device, texData->width, texData->height, mipCount);
	}
	else
	{
		device->TransitionImageLayout(commandBuffer, this, ResourceState::TransferDst, stateAfter, 0, mipCount);
	}

	device->EndSingleTimeCommands(commandBuffer);
}

void VulkanTexture::CreateResource(VulkanDevice* device, const uint32_t width, const uint32_t height, uint32_t arraySize, MultisampleType mstype)
{
	InitializeRegion(width, height, arraySize, 1);

	VulkanImageInfo textureResourceInfo;
	textureResourceInfo.Flags = (IsFlagSet(m_Flags & TextureFlags::CubeMap) ? ImageFlags::CubeMap : ImageFlags::None) |
		(IsFlagSet(m_Flags & TextureFlags::LinearTiling) ? ImageFlags::LinearTiling : ImageFlags::None);

	textureResourceInfo.UsageFlags = 
		(IsFlagSet(m_Flags & TextureFlags::TransferDst) ? ImageUsageFlags::TransferDst : ImageUsageFlags::None) |
		(IsFlagSet(m_Flags & TextureFlags::TransferSrc) ? ImageUsageFlags::TransferSrc : ImageUsageFlags::None) |
		(IsFlagSet(m_Flags & TextureFlags::DepthStencil) ? ImageUsageFlags::DepthStencil : ImageUsageFlags::Storage) | //TODO: investigate this storage flag
		(IsFlagSet(m_Flags & TextureFlags::Read) ? ImageUsageFlags::Resource : ImageUsageFlags::None) |
		(IsFlagSet(m_Flags & TextureFlags::RenderTarget) ? ImageUsageFlags::RenderTarget : ImageUsageFlags::None);

	textureResourceInfo.MemoryAccess = MemoryAccess::GPU;
	textureResourceInfo.Format = m_Format;
	textureResourceInfo.Extent.Width = width;
	textureResourceInfo.Extent.Height = height;
	textureResourceInfo.Extent.Depth = 1;
	textureResourceInfo.ArraySize = arraySize;
	textureResourceInfo.MipLevels = 1;
	textureResourceInfo.MultisampleType = mstype;
	m_Resource = new VulkanImage(device, textureResourceInfo, m_Name.c_str());

	ResourceState stateAfter = ResourceState::None;
	if (IsFlagSet(m_Flags & TextureFlags::Read) && IsFlagSet(m_Flags & TextureFlags::DepthStencil))
	{
		stateAfter = ResourceState::DepthStencilRead;
	}
	else if (IsFlagSet(m_Flags & TextureFlags::Read))
	{
		stateAfter = ResourceState::GenericRead;
	}
	else if (IsFlagSet(m_Flags & TextureFlags::RenderTarget) && IsFlagSet(m_Flags & TextureFlags::DepthStencil))
	{
		stateAfter = ResourceState::DepthStencilWrite;
	}
	else if (IsFlagSet(m_Flags & TextureFlags::RenderTarget))
	{
		stateAfter = ResourceState::RenderTarget;
	}

	VkCommandBuffer commandBuffer = device->BeginSingleTimeCommands();

	device->TransitionImageLayout(commandBuffer, this, ResourceState::None, stateAfter);

	m_ShouldBeResourceState = m_CurrentResourceState = stateAfter;

	device->EndSingleTimeCommands(commandBuffer);
}

void VulkanTexture::CreateView(VulkanDevice* device)
{
	ClearValue clearValue;
	if (IsFlagSet(m_Flags & TextureFlags::DepthStencil))
	{
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;
	}
	else
	{
		clearValue.Color.value[0] = 0.0f;
		clearValue.Color.value[1] = 0.0f;
		clearValue.Color.value[2] = 0.0f;
		clearValue.Color.value[3] = 1.0f;
	}

	VulkanImageViewInfo imageViewInfo;
	imageViewInfo.Resource = m_Resource;
	imageViewInfo.Format = Format::UNDEFINED;
	imageViewInfo.Flags = ImageViewFlags::Color;
	imageViewInfo.Subresource.MipSlice = 0;
	imageViewInfo.Subresource.MipSize = m_Region.Subresource.MipSize;
	imageViewInfo.Subresource.ArraySlice = 0;
	imageViewInfo.Subresource.ArraySize = m_Region.Subresource.ArraySize;
	imageViewInfo.ClearValue = clearValue;

	m_View = new VulkanImageView(device, imageViewInfo, m_Name.c_str());
}

void VulkanTexture::CreateSampler(VulkanDevice* device)
{
	VulkanImageSamplerInfo imageSamplerInfo;
	imageSamplerInfo.AddressModeU = AddressMode::Repeat;
	imageSamplerInfo.AddressModeV = AddressMode::Repeat;
	imageSamplerInfo.AddressModeW = AddressMode::Repeat;
	imageSamplerInfo.MagFilterType = FilterType::Anisotropic;
	imageSamplerInfo.MinFilterType = FilterType::Anisotropic;
	imageSamplerInfo.MipFilterType = FilterType::Linear;
	imageSamplerInfo.CompareOperation = CompareOperation::Never;
	imageSamplerInfo.MaxLevelOfAnisotropy = 16;
	imageSamplerInfo.BorderColor.value[0] = 0.0f;
	imageSamplerInfo.BorderColor.value[1] = 0.0f;
	imageSamplerInfo.BorderColor.value[2] = 0.0f;
	imageSamplerInfo.BorderColor.value[3] = 0.0f;
	imageSamplerInfo.MipLODBias = 0.0f;
	imageSamplerInfo.MinLOD = 0.f;
	imageSamplerInfo.MaxLOD = static_cast<float>(m_Region.Subresource.MipSize);

	m_Sampler = new VulkanImageSampler(device, imageSamplerInfo, m_Name.c_str());
}

void VulkanTexture::InitializeRegion(const uint32_t width, const uint32_t height, uint32_t arraySize, uint32_t mipCount)
{
	m_Region.Extent.Width = width;
	m_Region.Extent.Height = height;
	m_Region.Extent.Depth = 1;
	m_Region.Offset.X = 0;
	m_Region.Offset.Y = 0;
	m_Region.Offset.Z = 0;
	m_Region.Subresource.MipSlice = 0;
	m_Region.Subresource.MipSize = mipCount;
	m_Region.Subresource.ArraySlice = 0;
	m_Region.Subresource.ArraySize = arraySize;
}

void VulkanTexture::GenerateMips(VkCommandBuffer commandBuffer, VulkanDevice* device, const uint32_t width, const uint32_t height, uint32_t mipCount)
{
	// Check if image format supports linear blitting
// 	VkFormatProperties formatProperties;
// 	vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

	//if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
	//	throw std::runtime_error("texture image format does not support linear blitting!");
	//}

	int32_t mipWidth = width;
	int32_t mipHeight = height;

	for (uint32_t i = 1; i < mipCount; i++) 
	{
		device->TransitionImageLayout(commandBuffer, this, ResourceState::TransferDst, ResourceState::TransferSrc, i - 1);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer,
			m_Resource->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_Resource->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		device->TransitionImageLayout(commandBuffer, this, ResourceState::TransferSrc, m_ShouldBeResourceState, i - 1);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	device->TransitionImageLayout(commandBuffer, this, ResourceState::TransferDst, m_ShouldBeResourceState, mipCount - 1);
}
