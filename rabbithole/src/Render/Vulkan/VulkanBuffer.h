#pragma once

#include "VulkanTypes.h"

#define BUFFER_ID_START_POS 1024

struct VulkanBufferInfo
{
	BufferUsageFlags usageFlags;
	MemoryAccess	 memoryAccess;
	uint32_t		 size;
};

class VulkanBuffer
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
	inline uint32_t					GetId() { return m_Id; }

private:
	void CreateBufferResource();
	
private:
	const VulkanDevice*		m_Device;
	VulkanBufferInfo		m_Info;
	VkBuffer				m_Buffer;
	VmaAllocation			m_VmaAllocation;
	void*					m_HostVisibleData = nullptr;

	uint32_t				m_Id;
	static uint32_t			ms_currentId;

};