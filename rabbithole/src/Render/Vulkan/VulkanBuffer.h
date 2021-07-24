#pragma once

#include "VulkanTypes.h"

struct VulkanBufferInfo
{
	BufferUsageFlags usageFlags;
	MemoryAccess memoryAccess;
	uint32_t size;
};

class VulkanBuffer
{

public:
	VulkanBuffer(const VulkanDevice* device, const VulkanBufferInfo& info);
	~VulkanBuffer();

public:
	void* Map();
	void Unmap();

public:
	inline const VulkanBufferInfo GetInfo() const { return m_Info; }
	inline void* GetHostVisibleData() { return m_HostVisibleData; }
	inline VkBuffer GetBuffer() { return m_Buffer; }

private:
	const VulkanDevice* m_Device;
	VulkanBufferInfo m_Info;
	VkBuffer m_Buffer;
	VmaAllocation m_VmaAllocation;

	void* m_HostVisibleData = nullptr;

};