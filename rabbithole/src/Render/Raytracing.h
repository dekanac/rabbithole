#pragma once

#if defined(VULKAN_HWRT)

#include <vulkan/vulkan.h>
#include "Vulkan/VulkanBuffer.h"
#include "Vulkan/VulkanDevice.h"
#include "Render/Resource.h"

class Renderer;

namespace RayTracing
{
	uint64_t GetBufferDeviceAddress(VulkanDevice& device, VulkanBuffer* buffer);
	void InitRTFunctions(VulkanDevice& device);

	struct AccelerationStructure : public AllocatedResource
	{
		VkAccelerationStructureKHR accelerationStructure;
		uint64_t deviceAddress = 0;
		VulkanBuffer* buffer;
	};

	struct ShaderBindingTable
	{
		VulkanBuffer* buffer;
		VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegion{};
	};

	struct ShaderBindingTables
	{
		ShaderBindingTable raygen;
		ShaderBindingTable hit;
		ShaderBindingTable miss;
	};

	struct ScratchBuffer
	{
		uint64_t deviceAddress = 0;
		VulkanBuffer* buffer;
	};

	VkStridedDeviceAddressRegionKHR GetSbtEntryStridedDeviceAddressRegion(VulkanDevice& device, VulkanBuffer* buffer, uint32_t handleCount);

	uint64_t AlignedSize(uint64_t value, uint64_t alignment);

	void CreateAccelerationStructure(Renderer* renderer, AccelerationStructure& accelerationStructure, VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);
	ShaderBindingTable CreateShaderBindingTable(VulkanDevice& device, uint32_t handleCount);
}

#endif