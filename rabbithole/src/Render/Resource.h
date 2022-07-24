#pragma once

#include "Render/Vulkan/VulkanTypes.h"
#include "Render/Vulkan/VulkanDevice.h"

#include <memory>
#include <string>

class VulkanTexture;
class VulkanBuffer;

class AllocatedResource
{
public:
	AllocatedResource();
	virtual uint32_t GetID() { return m_Id; }

	static uint32_t ms_CurrentId;
protected:
	uint32_t m_Id;
	void UpdateID();
};

class ManagableResource
{
public:

	virtual ResourceState			GetResourceState() const = 0;
	virtual void					SetResourceState(ResourceState state) = 0;

	virtual ResourceState			GetShouldBeResourceState() const = 0;
	virtual void					SetShouldBeResourceState(ResourceState state) = 0;

	virtual ResourceStage			GetCurrentResourceStage() = 0;
	virtual void					SetCurrentResourceStage(ResourceStage stage) = 0;

	virtual ResourceStage			GetPreviousResourceStage() = 0;
	virtual void					SetPreviousResourceStage(ResourceStage stage) = 0;
};

struct ROTextureCreateInfo
{
	std::string		filePath;
	TextureFlags	flags = TextureFlags::None;
	Format			format = Format::UNDEFINED;
	std::string		name = "ROTexture";
	bool			isCube = false;
	bool			generateMips = false;
	SamplerType     samplerType = SamplerType::Anisotropic;
	AddressMode		addressMode = AddressMode::Repeat;
};

struct RWTextureCreateInfo
{
	Extent3D		dimensions = { 1, 1, 1 };
	TextureFlags	flags = TextureFlags::None;
	Format			format = Format::UNDEFINED;
	std::string		name = "RWTexture";
	uint32_t		arraySize = 1;
	bool			isCube;
	MultisampleType multisampleType = MultisampleType::Sample_1;
	SamplerType     samplerType = SamplerType::Anisotropic;
	AddressMode		addressMode = AddressMode::Repeat;
};

struct BufferCreateInfo
{
	BufferUsageFlags	flags = BufferUsageFlags::None;
	MemoryAccess		memoryAccess = MemoryAccess::CPU;
	uint32_t			size = 0;
	std::string			name = "Buffer";
};

