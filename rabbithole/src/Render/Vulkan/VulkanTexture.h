#pragma once

#include "Render/Model/TextureLoading.h"
#include "Render/Resource.h"
#include "Render/Vulkan/VulkanDevice.h"
#include "Render/Vulkan/VulkanImage.h"

class VulkanImage;
class VulkanImageView;
class VulkanImageSampler;

using namespace TextureLoading;

class VulkanTexture : public ManagableResource, public AllocatedResource
{
public:

	friend class ResourceManager; //Resource Manager will take care of creation and deletion of textures
private:
	VulkanTexture(VulkanDevice& device, RWTextureCreateInfo& createInfo);
	VulkanTexture(VulkanDevice& device, const TextureData* data, ROTextureCreateInfo& createInfo);
	~VulkanTexture();

public:
	VulkanImage*			GetResource() const { return m_Resource; }
	VulkanImageView*		GetView() const { return m_View; }
	VulkanImageSampler*		GetSampler() const { return m_Sampler; }
	Format					GetFormat() const{ return m_Format; }
	TextureFlags			GeFlags() const { return m_Flags; }
	ImageRegion				GetRegion() const { return m_Region; }
	std::string 			GetName() const { return m_Name; }

	virtual ResourceState			GetResourceState() const { return m_CurrentResourceState; };
	virtual void					SetResourceState(ResourceState state) { m_CurrentResourceState = state; }

	virtual ResourceState			GetShouldBeResourceState() const { return m_ShouldBeResourceState; }
	virtual void					SetShouldBeResourceState(ResourceState state) { m_ShouldBeResourceState = state; }

	virtual ResourceStage			GetCurrentResourceStage() { return m_CurrentResourceStage; }
	virtual void					SetCurrentResourceStage(ResourceStage stage) { m_CurrentResourceStage = stage; }

	virtual ResourceStage			GetPreviousResourceStage() { return m_PreviousResourceStage; }
	virtual void					SetPreviousResourceStage(ResourceStage stage) { m_PreviousResourceStage = stage; }

	uint32_t				GetWidth() const { return m_Region.Extent.Width; }
	uint32_t				GetHeight() const { return m_Region.Extent.Height; }
	uint32_t				GetDepth() const { return m_Region.Extent.Depth; }

private:
	void CreateResource(VulkanDevice* device, const TextureData* texData, bool generateMips = false);
	void CreateResource(VulkanDevice* device, const uint32_t width, const uint32_t height, const uint32_t depth, uint32_t arraySize = 1, MultisampleType mstype = MultisampleType::Sample_1);
	void CreateView(VulkanDevice* device);
	void CreateSampler(VulkanDevice* device, SamplerType type, AddressMode addressMode);
	void InitializeRegion(const uint32_t width, const uint32_t height, const uint32_t depth, uint32_t arraySize = 1, uint32_t mipCount = 1);
	void GenerateMips(VulkanCommandBuffer& commandBuffer, VulkanDevice* device, const uint32_t width, const uint32_t height, uint32_t mipCount);

private:
	VulkanImage*			m_Resource;
	VulkanImageView*		m_View;
	VulkanImageSampler*		m_Sampler;

	ImageRegion				m_Region;
	Format					m_Format;
	TextureFlags			m_Flags;
	std::string				m_Name = "DefaultTextureName";

	ResourceState			m_CurrentResourceState = ResourceState::Count;
	ResourceState			m_ShouldBeResourceState = ResourceState::Count;

	ResourceStage			m_CurrentResourceStage = ResourceStage::Count;
	ResourceStage			m_PreviousResourceStage = ResourceStage::Count;
};