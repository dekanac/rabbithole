#pragma once

#include "Render/Vulkan/VulkanTypes.h"
#include "Render/Vulkan/VulkanDevice.h"
#include "Render/Model/TextureLoading.h"

#include <memory>
#include <string>

using TextureData = TextureLoading::TextureData;

class VulkanTexture;
class VulkanBuffer;

class AllocatedResource
{
public:
	AllocatedResource();

	virtual uint32_t GetID() { return m_Id; }

	static uint32_t ms_CurrentId;

protected:
	void UpdateID();

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

	ResourceType			GetType() const { return m_Type; }

	virtual ResourceState	GetResourceState() const { return m_CurrentResourceState; };
	virtual void			SetResourceState(ResourceState state) { m_CurrentResourceState = state; }

	virtual ResourceState	GetShouldBeResourceState() const { return m_ShouldBeResourceState; }
	virtual void			SetShouldBeResourceState(ResourceState state) { m_ShouldBeResourceState = state; }

	virtual ResourceStage	GetCurrentResourceStage() { return m_CurrentResourceStage; }
	virtual void			SetCurrentResourceStage(ResourceStage stage) { m_PreviousResourceStage = m_CurrentResourceStage; m_CurrentResourceStage = stage; }

	virtual ResourceStage	GetPreviousResourceStage() { return m_PreviousResourceStage; }

protected:
	ResourceState			m_CurrentResourceState = ResourceState::Count;
	ResourceState			m_ShouldBeResourceState = ResourceState::Count;

	ResourceStage			m_CurrentResourceStage = ResourceStage::Count;
	ResourceStage			m_PreviousResourceStage = ResourceStage::Count;

	ResourceType m_Type;
};

struct ROTextureCreateInfo
{
	TextureFlags	flags = TextureFlags::None;
	Format			format = Format::UNDEFINED;
	std::string		name = "ROTexture";
	bool			isCube = false;
	bool			generateMips = false;
	SamplerType     samplerType = SamplerType::Trilinear;
	AddressMode		addressMode = AddressMode::Repeat;
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
	SamplerType     samplerType = SamplerType::Anisotropic;
	AddressMode		addressMode = AddressMode::Repeat;
	uint32_t		mipCount = 1;
};

struct BufferCreateInfo
{
	BufferUsageFlags	flags = BufferUsageFlags::None;
	MemoryAccess		memoryAccess = MemoryAccess::CPU;
	uint32_t			size = 0;
	std::string			name = "Buffer";
};

