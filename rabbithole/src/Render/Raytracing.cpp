#include "common.h"

#include "Raytracing.h"
#include "Render/Renderer.h"

#if defined(VULKAN_HWRT)

namespace RayTracing
{
	PFN_vkCreateAccelerationStructureKHR pfnvkCreateAccelerationStructureKHR;
	PFN_vkGetAccelerationStructureDeviceAddressKHR pfnvkGetAccelerationStructureDeviceAddressKHR;

	void InitRTFunctions(VulkanDevice& device)
	{
		pfnvkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device.GetGraphicDevice(), "vkCreateAccelerationStructureKHR"));
		pfnvkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device.GetGraphicDevice(), "vkGetAccelerationStructureDeviceAddressKHR"));
	}

	uint64_t AlignedSize(uint64_t value, uint64_t alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}

	uint64_t GetBufferDeviceAddress(VulkanDevice& device, VulkanBuffer* buffer)
	{
		VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{};
		bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAddressInfo.buffer = GET_VK_HANDLE_PTR(buffer);
		return vkGetBufferDeviceAddress(device.GetGraphicDevice(), &bufferDeviceAddressInfo);
	}

	void CreateAccelerationStructure(Renderer* renderer, AccelerationStructure& accelerationStructure, VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo)
	{
		VulkanBuffer* asBuffer = renderer->GetResourceManager().CreateBuffer(renderer->GetVulkanDevice(), BufferCreateInfo{
				.flags = { BufferUsageFlags::AccelerationStructureStorage | BufferUsageFlags::ShaderDeviceAddress },
				.memoryAccess = { MemoryAccess::GPU },
				.size = { static_cast<uint32_t>(buildSizeInfo.accelerationStructureSize)},
				.name = {"Denoise Dimensions buffer"}
			});
		// Acceleration structure
		VkAccelerationStructureCreateInfoKHR accelerationStructureCreate_info{};
		accelerationStructureCreate_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureCreate_info.buffer = GET_VK_HANDLE_PTR(asBuffer);
		accelerationStructureCreate_info.size = buildSizeInfo.accelerationStructureSize;
		accelerationStructureCreate_info.type = type;
		pfnvkCreateAccelerationStructureKHR(renderer->GetVulkanDevice().GetGraphicDevice(), &accelerationStructureCreate_info, nullptr, &accelerationStructure.accelerationStructure);
		// AS device address
		VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
		accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		accelerationDeviceAddressInfo.accelerationStructure = accelerationStructure.accelerationStructure;
		accelerationStructure.deviceAddress = pfnvkGetAccelerationStructureDeviceAddressKHR(renderer->GetVulkanDevice().GetGraphicDevice(), &accelerationDeviceAddressInfo);
	}

	RayTracing::ShaderBindingTable CreateShaderBindingTable(VulkanDevice& device, uint32_t handleCount)
	{
		ShaderBindingTable newTable{};
		auto& resourceManager = Renderer::instance().GetResourceManager();
		auto shaderGroupHandleSize = device.GetRayTracingProperties().shaderGroupHandleSize;

		newTable.buffer = resourceManager.CreateBuffer(device, BufferCreateInfo{
				.flags = { BufferUsageFlags::ShaderBindingTable | BufferUsageFlags::ShaderDeviceAddress},
				.memoryAccess = { MemoryAccess::CPU },
				.size = {shaderGroupHandleSize * handleCount},
				.name = {"Table Buffer"}
			});
		newTable.stridedDeviceAddressRegion = RayTracing::GetSbtEntryStridedDeviceAddressRegion(device, newTable.buffer, handleCount);
		newTable.buffer->Map();

		return newTable;
	}

	VkStridedDeviceAddressRegionKHR GetSbtEntryStridedDeviceAddressRegion(VulkanDevice& device, VulkanBuffer* buffer, uint32_t handleCount)
	{
		const uint32_t handleSizeAligned = AlignedSize(device.GetRayTracingProperties().shaderGroupHandleSize, device.GetRayTracingProperties().shaderGroupHandleAlignment);
		VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegionKHR{};
		stridedDeviceAddressRegionKHR.deviceAddress = GetBufferDeviceAddress(device, buffer);
		stridedDeviceAddressRegionKHR.stride = handleSizeAligned;
		stridedDeviceAddressRegionKHR.size = handleCount * handleSizeAligned;
		return stridedDeviceAddressRegionKHR;
	}
	
}

#endif

