#include "precomp.h"

#include "Render/Converters.h"
#include "Render/Model/Model.h"
#include "Render/Model/TextureLoading.h"

VulkanTexture::VulkanTexture(VulkanDevice& device, RWTextureCreateInfo& createInfo)
	: ManagableResource(ResourceType::Texture)
	, m_Format(createInfo.format)
	, m_Flags(createInfo.flags)
	, m_Name(createInfo.name)
{
	CreateResource(&device, createInfo);
	CreateView(&device);
	CreateSampler(&device, createInfo.samplerType, createInfo.addressMode);

	device.SetObjectName((uint64_t)GET_VK_HANDLE_PTR(m_Resource), VK_OBJECT_TYPE_IMAGE, createInfo.name.c_str());
	device.SetObjectName((uint64_t)GET_VK_HANDLE_PTR(m_View), VK_OBJECT_TYPE_IMAGE_VIEW, createInfo.name.c_str());
	device.SetObjectName((uint64_t)GET_VK_HANDLE_PTR(m_Sampler), VK_OBJECT_TYPE_SAMPLER, createInfo.name.c_str());
}

VulkanTexture::VulkanTexture(VulkanDevice& device, const TextureData* data, ROTextureCreateInfo& createInfo)
	: ManagableResource(ResourceType::Texture)
	, m_Format(createInfo.format)
	, m_Flags(createInfo.flags)
	, m_Name(createInfo.name)
{
	CreateResource(&device, data, createInfo.generateMips);
	CreateView(&device);
	CreateSampler(&device, createInfo.samplerType, createInfo.addressMode);

	device.SetObjectName((uint64_t)GET_VK_HANDLE_PTR(m_Resource), VK_OBJECT_TYPE_IMAGE, createInfo.name.c_str());
	device.SetObjectName((uint64_t)GET_VK_HANDLE_PTR(m_View), VK_OBJECT_TYPE_IMAGE_VIEW, createInfo.name.c_str());
	device.SetObjectName((uint64_t)GET_VK_HANDLE_PTR(m_Sampler), VK_OBJECT_TYPE_SAMPLER, createInfo.name.c_str());
}

VulkanTexture::VulkanTexture(VulkanDevice& device, const VulkanTexture* other, uint32_t mipSlice)
	: ManagableResource((ManagableResource)*other)
	, m_Resource(other->m_Resource)
	, m_Sampler(other->m_Sampler)
	, m_Format(other->m_Format)
	, m_Flags(other->m_Flags)
	, m_Name(std::format("{}_mip{}", other->m_Name, mipSlice))
{
	uint32_t mipWidth = other->m_Region.Extent.Width;
	uint32_t mipHeight = other->m_Region.Extent.Height;

	uint32_t maxMipLevels = GET_MIP_LEVELS_FROM_RES(mipWidth, mipHeight);
	ASSERT(mipSlice <= maxMipLevels, "More mips than its possible");

	for (uint32_t i = 0; i < mipSlice; i++)
	{
		mipWidth >>= 1;
		mipHeight >>= 1;
	}

	m_Region.Subresource.ArraySize = 1;
	m_Region.Subresource.ArraySlice = 0;
	m_Region.Subresource.MipSize = 1;
	m_Region.Subresource.MipSlice = mipSlice;

	m_Region.Extent.Height = mipHeight;
	m_Region.Extent.Width = mipWidth;
	m_Region.Extent.Depth = other->m_Region.Extent.Depth;

	m_Region.Offset = other->m_Region.Offset;

	VulkanImageViewInfo imageViewInfo;
	imageViewInfo.Resource = m_Resource;
	imageViewInfo.Format = m_Format;
	imageViewInfo.Subresource.MipSlice = mipSlice;
	imageViewInfo.Subresource.MipSize = m_Region.Subresource.MipSize;
	imageViewInfo.Subresource.ArraySlice = 0;
	imageViewInfo.Subresource.ArraySize = 1;
	imageViewInfo.ClearValue = GetClearColorValueFor(m_Format);

	m_View = new VulkanImageView(&device, imageViewInfo, m_Name.c_str());

	device.SetObjectName((uint64_t)GET_VK_HANDLE_PTR(m_View), VK_OBJECT_TYPE_IMAGE_VIEW, m_Name.c_str());
}

VulkanTexture::~VulkanTexture()
{
	if (m_Sampler) { delete(m_Sampler); m_Sampler = nullptr; }
	if (m_View) { delete(m_View); m_View = nullptr; }
	if (m_Resource) { delete(m_Resource); m_Resource = nullptr; }
}

void VulkanTexture::CreateResource(VulkanDevice* device, const TextureData* texData, bool generateMips)
{
	bool isCubeMap = IsFlagSet(m_Flags & TextureFlags::CubeMap);

	uint32_t mipCount = generateMips ? GET_MIP_LEVELS_FROM_RES(texData->width, texData->height) : 1;
	uint32_t arraySize = isCubeMap ? 6 : 1;

	InitializeRegion(Extent3D{ static_cast<uint32_t>(texData->width), static_cast<uint32_t>(texData->height), 1 }, arraySize, mipCount);

	uint32_t textureSize = texData->height * texData->width * GetBPPFrom(m_Format) * arraySize;

	VulkanBuffer stagingBuffer(*device, BufferUsageFlags::TransferSrc, MemoryAccess::CPU, textureSize, "StagingBuffer");

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
		(IsFlagSet(m_Flags & TextureFlags::RenderTarget) ? ImageUsageFlags::RenderTarget : ImageUsageFlags::None) |
		(IsFlagSet(m_Flags & TextureFlags::Storage) ? ImageUsageFlags::Storage : ImageUsageFlags::None);
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
	if (IsFlagSet(m_Flags & TextureFlags::DepthStencil))
	{
		stateAfter = ResourceState::DepthStencilWrite;
	}
	else if (IsFlagSet(m_Flags & TextureFlags::Read))
	{
		stateAfter = ResourceState::GenericRead;
	}
	else if (IsFlagSet(m_Flags & TextureFlags::RenderTarget))
	{
		stateAfter = ResourceState::RenderTarget;
	}

	VulkanCommandBuffer tempCommandBuffer(*device, "Temp Texture Creation Command Buffer");
	tempCommandBuffer.BeginCommandBuffer(true);

	device->ResourceBarrier(tempCommandBuffer, this, ResourceState::None, ResourceState::TransferDst, ResourceStage::Undefined, ResourceStage::Transfer, 0, mipCount);

	if (isCubeMap)
	{
		device->CopyBufferToImageCubeMap(tempCommandBuffer, &stagingBuffer, this);
	}
	else
	{
		device->CopyBufferToImage(tempCommandBuffer, &stagingBuffer, this, true);
	}

	m_ShouldBeResourceState = m_CurrentResourceState = stateAfter;

	if (generateMips)
	{
		GenerateMips(tempCommandBuffer, device, mipCount);
	}
	else
	{
		device->ResourceBarrier(tempCommandBuffer, this, ResourceState::TransferDst, stateAfter, ResourceStage::Transfer, ResourceStage::Undefined);
	}

	tempCommandBuffer.EndAndSubmitCommandBuffer();
}

void VulkanTexture::CreateResource(VulkanDevice* device, RWTextureCreateInfo& createInfo)
{
	InitializeRegion(createInfo.dimensions, createInfo.arraySize, createInfo.mipCount);

	VulkanImageInfo textureResourceInfo;
	textureResourceInfo.Flags = (IsFlagSet(m_Flags & TextureFlags::CubeMap) ? ImageFlags::CubeMap : ImageFlags::None) |
		(IsFlagSet(m_Flags & TextureFlags::LinearTiling) ? ImageFlags::LinearTiling : ImageFlags::None);

	textureResourceInfo.UsageFlags = 
		(IsFlagSet(m_Flags & TextureFlags::TransferDst) ? ImageUsageFlags::TransferDst : ImageUsageFlags::None) |
		(IsFlagSet(m_Flags & TextureFlags::TransferSrc) ? ImageUsageFlags::TransferSrc : ImageUsageFlags::None) |
		(IsFlagSet(m_Flags & TextureFlags::DepthStencil) ? ImageUsageFlags::DepthStencil : ImageUsageFlags::None) | //TODO: investigate this storage flag
		(IsFlagSet(m_Flags & TextureFlags::Read) ? ImageUsageFlags::Resource : ImageUsageFlags::None) |
		(IsFlagSet(m_Flags & TextureFlags::RenderTarget) ? ImageUsageFlags::RenderTarget : ImageUsageFlags::None) |
		(IsFlagSet(m_Flags & TextureFlags::Storage) ? ImageUsageFlags::Storage : ImageUsageFlags::None);

	textureResourceInfo.MemoryAccess = MemoryAccess::GPU;
	textureResourceInfo.Format = m_Format;
	textureResourceInfo.Extent= createInfo.dimensions;
	textureResourceInfo.ArraySize = createInfo.arraySize;
	textureResourceInfo.MipLevels = createInfo.mipCount;
	textureResourceInfo.MultisampleType = createInfo.multisampleType;

	m_Resource = new VulkanImage(device, textureResourceInfo, m_Name.c_str());

	ResourceState stateAfter = ResourceState::None;
	if (IsFlagSet(m_Flags & TextureFlags::DepthStencil))
	{
		stateAfter = ResourceState::DepthStencilWrite;
	}
	else if (IsFlagSet(m_Flags & TextureFlags::Read))
	{
		stateAfter = ResourceState::GenericRead;
	}
	else if (IsFlagSet(m_Flags & TextureFlags::RenderTarget))
	{
		stateAfter = ResourceState::RenderTarget;
	}

	VulkanCommandBuffer tempCommandBuffer(*device, "Temp Texture Creation Command Buffer");
	tempCommandBuffer.BeginCommandBuffer(true);

	device->ResourceBarrier(tempCommandBuffer, this, ResourceState::None, stateAfter, ResourceStage::Undefined, ResourceStage::Undefined, 0, m_Region.Subresource.MipSize);

	m_ShouldBeResourceState = m_CurrentResourceState = stateAfter;

	tempCommandBuffer.EndAndSubmitCommandBuffer();
}

void VulkanTexture::CreateView(VulkanDevice* device)
{
	VulkanImageViewInfo imageViewInfo;
	imageViewInfo.Resource = m_Resource;
	imageViewInfo.Format = m_Format;
	imageViewInfo.Subresource.MipSlice = 0;
	imageViewInfo.Subresource.MipSize = m_Region.Subresource.MipSize;
	imageViewInfo.Subresource.ArraySlice = 0;
	imageViewInfo.Subresource.ArraySize = m_Region.Subresource.ArraySize;
	imageViewInfo.ClearValue = GetClearColorValueFor(m_Format);

	m_View = new VulkanImageView(device, imageViewInfo, m_Name.c_str());
}

void VulkanTexture::CreateSampler(VulkanDevice* device, SamplerType type, AddressMode addressMode)
{
	VulkanImageSamplerInfo imageSamplerInfo;
	imageSamplerInfo.AddressModeU = addressMode;
	imageSamplerInfo.AddressModeV = addressMode;
	imageSamplerInfo.AddressModeW = addressMode;
	imageSamplerInfo.MagFilterType = (type == SamplerType::Point) ? FilterType::Point : FilterType::Linear;
	imageSamplerInfo.MinFilterType = (type == SamplerType::Point) ? FilterType::Point : FilterType::Linear;
	imageSamplerInfo.MipFilterType = (type == SamplerType::Point) ? FilterType::Point : FilterType::Linear;
	imageSamplerInfo.CompareOperation = CompareOperation::Never;
	imageSamplerInfo.MaxLevelOfAnisotropy = type == SamplerType::Anisotropic ? 16 : 0;
	imageSamplerInfo.BorderColor.value[0] = 0.0f;
	imageSamplerInfo.BorderColor.value[1] = 0.0f;
	imageSamplerInfo.BorderColor.value[2] = 0.0f;
	imageSamplerInfo.BorderColor.value[3] = 0.0f;
	imageSamplerInfo.MipLODBias = 0.0f;
	imageSamplerInfo.MinLOD = 0.f;
	imageSamplerInfo.MaxLOD = static_cast<float>(m_Region.Subresource.MipSize);

	m_Sampler = new VulkanImageSampler(device, imageSamplerInfo, m_Name.c_str());
}

void VulkanTexture::InitializeRegion(Extent3D dimensions, uint32_t arraySize, uint32_t mipCount)
{
	m_Region.Extent = dimensions;
	m_Region.Offset.X = 0;
	m_Region.Offset.Y = 0;
	m_Region.Offset.Z = 0;
	m_Region.Subresource.MipSlice = 0;
	m_Region.Subresource.MipSize = mipCount;
	m_Region.Subresource.ArraySlice = 0;
	m_Region.Subresource.ArraySize = arraySize;
}

void VulkanTexture::GenerateMips(VulkanCommandBuffer& commandBuffer, VulkanDevice* device, uint32_t mipCount)
{
	// Check if image format supports linear blitting
 	VkFormatProperties formatProperties;
 	vkGetPhysicalDeviceFormatProperties(device->GetPhysicalDevice(), GetVkFormatFrom(m_Format), &formatProperties);

	ASSERT(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT, "Format doesn't support linear filtering and you're trying to generate mips");

	int32_t mipWidth = m_Region.Extent.Width;
	int32_t mipHeight = m_Region.Extent.Height;

	for (uint32_t i = 1; i < mipCount; i++) 
	{
		device->ResourceBarrier(commandBuffer, this, ResourceState::TransferDst, ResourceState::TransferSrc, ResourceStage::Transfer, ResourceStage::Transfer, i - 1, 1);

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

		vkCmdBlitImage(GET_VK_HANDLE(commandBuffer),
			GET_VK_HANDLE_PTR(m_Resource), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			GET_VK_HANDLE_PTR(m_Resource), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		device->ResourceBarrier(commandBuffer, this, ResourceState::TransferSrc, m_ShouldBeResourceState, ResourceStage::Transfer, ResourceStage::Undefined, i - 1, 1);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	device->ResourceBarrier(commandBuffer, this, ResourceState::TransferDst, m_ShouldBeResourceState, ResourceStage::Transfer, ResourceStage::Undefined, mipCount - 1, 1);
}
