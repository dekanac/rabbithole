#include "precomp.h"

#include "Model.h"

VulkanTexture::VulkanTexture(VulkanDevice* device, std::string filePath, TextureFlags flags, Format format, std::string name)
	: m_Format(format)
	, m_Flags(flags)
	, m_FilePath(filePath)
	, m_Name(name)
{
	auto texData = ModelLoading::LoadTexture(filePath);

	CreateResource(device, texData);
	CreateView(device);
	CreateSampler(device);

	ModelLoading::FreeTexture(texData);
}

VulkanTexture::VulkanTexture(VulkanDevice* device, const uint32_t width, const uint32_t height, TextureFlags flags, Format format, const char* name)
	: m_Format(format)
	, m_Flags(flags)
	, m_FilePath("")
	, m_Name(name)
{
	CreateResource(device, width, height);
	CreateView(device);
	CreateSampler(device);

	//device->SetObjectName((uint64_t)(m_Resource->GetImage()), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, name);
	//device->SetObjectName((uint64_t)m_View->GetImageView(), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, name);
	//device->SetObjectName((uint64_t)m_Sampler->GetSampler(), VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, name);
}

VulkanTexture::VulkanTexture(VulkanDevice* device, TextureData* texData, TextureFlags flags, Format format, std::string name)
	: m_Format(format)
	, m_Flags(flags)
	, m_FilePath("")
	, m_Name(name)
{
	
	CreateResource(device, texData);
	CreateView(device);
	CreateSampler(device);
}

VulkanTexture::~VulkanTexture()
{
	delete(m_Sampler);
	delete(m_View);
	delete(m_Resource);
}

void VulkanTexture::CreateResource(VulkanDevice* device, TextureData* texData)
{
	InitializeRegion(texData->width, texData->height);
	bool isCubeMap = IsFlagSet(m_Flags & TextureFlags::CubeMap);

	int textureSize = texData->height * texData->width * GetBPPFrom(m_Format);
	if (isCubeMap)
	{
		textureSize *= 6;
	}

	VulkanBufferInfo bufferInfo{};
	bufferInfo.memoryAccess = MemoryAccess::Host;
	bufferInfo.usageFlags = BufferUsageFlags::StorageBuffer | BufferUsageFlags::TransferSrc;
	bufferInfo.size = textureSize;

	VulkanBuffer stagingBuffer(device, bufferInfo);

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
	textureResourceInfo.MemoryAccess = MemoryAccess::Device;
	textureResourceInfo.Format = m_Format;
	textureResourceInfo.Extent.Width = texData->width;
	textureResourceInfo.Extent.Height = texData->height;
	textureResourceInfo.Extent.Depth = 1;
	textureResourceInfo.ArraySize = 1;
	textureResourceInfo.MipLevels = 1;
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

	device->TransitionImageLayout(this, ResourceState::None, ResourceState::TransferDst);

	if (isCubeMap)
	{
		device->CopyBufferToImageCubeMap(&stagingBuffer, this);
	}
	else
	{
		device->CopyBufferToImage(&stagingBuffer, this);
	}

	device->TransitionImageLayout(this, ResourceState::TransferDst, stateAfter);
}

void VulkanTexture::CreateResource(VulkanDevice* device, const uint32_t width, const uint32_t height)
{
	InitializeRegion(width, height);

	VulkanImageInfo textureResourceInfo;
	textureResourceInfo.Flags = (IsFlagSet(m_Flags & TextureFlags::CubeMap) ? ImageFlags::CubeMap : ImageFlags::None) |
		(IsFlagSet(m_Flags & TextureFlags::LinearTiling) ? ImageFlags::LinearTiling : ImageFlags::None);

	textureResourceInfo.UsageFlags = 
		(IsFlagSet(m_Flags & TextureFlags::DepthStencil) ? ImageUsageFlags::DepthStencil : ImageUsageFlags::Storage) | //TODO: investigate this storage flag
		(IsFlagSet(m_Flags & TextureFlags::Read) ? ImageUsageFlags::Resource : ImageUsageFlags::None) |
		(IsFlagSet(m_Flags & TextureFlags::RenderTarget) ? ImageUsageFlags::RenderTarget : ImageUsageFlags::None);

	textureResourceInfo.MemoryAccess = MemoryAccess::Device;
	textureResourceInfo.Format = m_Format;
	textureResourceInfo.Extent.Width = width;
	textureResourceInfo.Extent.Height = height;
	textureResourceInfo.Extent.Depth = 1;
	textureResourceInfo.ArraySize = 1;
	textureResourceInfo.MipLevels = 1;
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

	device->TransitionImageLayout(this, ResourceState::None, stateAfter);
}

void VulkanTexture::CreateResource(VulkanDevice* device)
{
	ASSERT(false, "Not implemented");
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
	imageViewInfo.Subresource.MipSize = 1;
	imageViewInfo.Subresource.ArraySlice = 0;
	imageViewInfo.Subresource.ArraySize = 1;
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
	imageSamplerInfo.MinLOD = 0.0f;
	imageSamplerInfo.MaxLOD = 0.0f;
	m_Sampler = new VulkanImageSampler(device, imageSamplerInfo, m_Name.c_str());
}

void VulkanTexture::InitializeRegion(const uint32_t width, const uint32_t height)
{
	m_Region.Extent.Width = width;
	m_Region.Extent.Height = height;
	m_Region.Extent.Depth = 1;
	m_Region.Offset.X = 0;
	m_Region.Offset.Y = 0;
	m_Region.Offset.Z = 0;
	m_Region.Subresource.MipSlice = 0;
	m_Region.Subresource.MipSize = 1;
	m_Region.Subresource.ArraySlice = 0;
	m_Region.Subresource.ArraySize = IsFlagSet(m_Flags & TextureFlags::CubeMap) ? 6 : 1;
}
