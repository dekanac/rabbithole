#pragma once

#include "VulkanTypes.h"

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

public:
	inline const VulkanBufferInfo	GetInfo() const { return m_Info; }
	inline void*					GetHostVisibleData() { return m_HostVisibleData; }
	inline VkBuffer					GetBuffer() { return m_Buffer; }

private:
	void CreateBufferResource();
	
private:
	const VulkanDevice*		m_Device;
	VulkanBufferInfo		m_Info;
	VkBuffer				m_Buffer;
	VmaAllocation			m_VmaAllocation;
	void*					m_HostVisibleData = nullptr;

};

class WrappedBuffer
{
public:
	WrappedBuffer(const VulkanDevice* device, BufferUsageFlags bufferType, uint32_t size, void* data);
	~WrappedBuffer();

	static void InitializeBuffer(VulkanDevice* device);
	static void BindBuffer(VulkanDevice* device);

private:

	const VulkanDevice* m_VulkanDevice;

public:
	static VulkanBuffer*	ms_MainGPUBuffer;
	static uint32_t			ms_CurrentOffset;
	static void*			ms_Data;

private:
	uint32_t m_Offset;
	uint32_t m_Size;

};