#pragma once

#include "VulkanTypes.h"
#include "Resource.h"

struct VulkanBufferInfo
{
	BufferUsageFlags usageFlags;
	MemoryAccess	 memoryAccess;
	uint32_t		 size;
};

class VulkanBuffer : public AllocatedResource
{
public:
	VulkanBuffer(const VulkanDevice* device, const VulkanBufferInfo& info);
	VulkanBuffer(const VulkanDevice* device, BufferUsageFlags flags, MemoryAccess access, uint64_t size);
	~VulkanBuffer();

public:
	void* Map();
	void  Unmap();
	void  FillBuffer(void* data, size_t size);

public:
	inline const VulkanBufferInfo	GetInfo() const { return m_Info; }
	inline void*					GetHostVisibleData() { return m_HostVisibleData; }
	inline VkBuffer					GetBuffer() { return m_Buffer; }
	uint32_t						GetID() { return m_Id; }
	inline size_t					GetSize() { return m_Size; }

private:
	void CreateBufferResource();
	
private:
	const VulkanDevice*		m_Device;
	VulkanBufferInfo		m_Info;
	VkBuffer				m_Buffer;
	VmaAllocation			m_VmaAllocation;
	void*					m_HostVisibleData = nullptr;

	size_t					m_Size;
};