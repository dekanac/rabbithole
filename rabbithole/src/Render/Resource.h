#pragma once

#include "Render/Vulkan/VulkanTypes.h"
#include "Render/Vulkan/VulkanDevice.h"
#include "Render/Model/TextureLoading.h"
#include "Render/Converters.h"

#include <memory>
#include <string>

using TextureData = TextureLoading::TextureData;

class VulkanTexture;
class VulkanBuffer;

class AllocatedResource
{
public:
	AllocatedResource();
	virtual ~AllocatedResource() {}

	uint32_t GetID() const { return m_Id; }

	static uint32_t ms_CurrentId;

private:
	uint32_t m_Id;
};

enum class ResourceType
{
	Buffer,
	Texture
};

class ManagableResource
{
public:
	ManagableResource(ResourceType type)
		: m_Type(type) {}

	ResourceType	GetType() const { return m_Type; }

	ResourceState	GetResourceState() const { return m_CurrentResourceState; };
	void			SetResourceState(ResourceState state) { m_CurrentResourceState = state; }

	ResourceState	GetShouldBeResourceState() const { return m_ShouldBeResourceState; }
	void			SetShouldBeResourceState(ResourceState state) { m_ShouldBeResourceState = state; }

	ResourceStage	GetCurrentResourceStage() { return m_CurrentResourceStage; }
	void			SetCurrentResourceStage(ResourceStage stage) { m_PreviousResourceStage = m_CurrentResourceStage; m_CurrentResourceStage = stage; }

	ResourceStage	GetPreviousResourceStage() { return m_PreviousResourceStage; }

protected:

	ResourceState	m_CurrentResourceState = ResourceState::Count;
	ResourceState	m_ShouldBeResourceState = ResourceState::Count;
					
	ResourceStage	m_CurrentResourceStage = ResourceStage::Count;
	ResourceStage	m_PreviousResourceStage = ResourceStage::Count;

	ResourceType m_Type;
};

struct ROTextureCreateInfo
{
	TextureFlags	flags = TextureFlags::None;
	Format			format = Format::UNDEFINED;
	std::string		name = "ROTexture";
	bool			isCube = false;
	bool			generateMips = false;
	SamplerType     samplerType = SamplerType::Bilinear;
	AddressMode		addressMode = AddressMode::Repeat;
	uint32_t		mipCount = 1;
};

struct RWTextureCreateInfo
{
	Extent3D		dimensions = { 1, 1, 1 };
	TextureFlags	flags = TextureFlags::None;
	Format			format = Format::UNDEFINED;
	std::string		name = "RWTexture";
	uint32_t		arraySize = 1;
	bool			isCube = false;
	MultisampleType multisampleType = MultisampleType::Sample_1;
	SamplerType     samplerType = SamplerType::Bilinear;
	AddressMode		addressMode = AddressMode::Repeat;
	uint32_t		mipCount = 1;
	ClearValue		clearValue = GetClearColorValueFor(format);
};

struct BufferCreateInfo
{
	BufferUsageFlags	flags = BufferUsageFlags::None;
	MemoryAccess		memoryAccess = MemoryAccess::CPU;
	uint32_t			size = 0;
	std::string			name = "Buffer";
};

