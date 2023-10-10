#pragma once

#include "VulkanTypes.h"
#include "Render/Resource.h"

class VulkanBuffer : public AllocatedResource, public ManagableResource
{
public:
	VulkanBuffer(VulkanDevice& device, BufferCreateInfo createInfo);
	~VulkanBuffer();

	NonCopyableAndMovable(VulkanBuffer);

	friend class ResourceManager; //Resource Manager will take care of creation and deletion of buffers
private:

public:
	void* Map();
	void  Unmap();
	void  FillBuffer(void* data);
	void  FillBuffer(void* data, uint64_t size, uint64_t offset = 0);

public:
	inline void*	GetHostVisibleData() { return m_HostVisibleData; }
	inline VkBuffer	GetVkHandle() { return m_Buffer; }
	inline uint64_t	GetSize() { return m_Info.size; }

private:
	void CreateBufferResource();
	
private:
	VulkanDevice&			m_Device;
	VkBuffer				m_Buffer;
	VmaAllocation			m_VmaAllocation;
	void*					m_HostVisibleData = nullptr;
	BufferCreateInfo		m_Info;
};