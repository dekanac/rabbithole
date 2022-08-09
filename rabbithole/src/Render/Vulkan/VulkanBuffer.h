#pragma once

#include "VulkanTypes.h"
#include "Render/Resource.h"

struct VulkanBufferInfo
{
	BufferUsageFlags usageFlags;
	MemoryAccess	 memoryAccess;
	uint64_t		 size;
};

class VulkanBuffer : public AllocatedResource
{
public:
	//TODO: take care of this, for now using this for staging buffers only
	VulkanBuffer(VulkanDevice& device, BufferUsageFlags flags, MemoryAccess access, uint64_t size, const char* name);
	~VulkanBuffer();

	friend class ResourceManager; //Resource Manager will take care of creation and deletion of buffers
private:
	VulkanBuffer(VulkanDevice& device, BufferCreateInfo& createInfo);

public:
	void* Map();
	void  Unmap();
	void  FillBuffer(void* data);
	void  FillBuffer(void* data, uint64_t size);
	void  FillBuffer(void* data, uint64_t offset, uint64_t size);

public:
	inline const VulkanBufferInfo	GetInfo() const { return m_Info; }
	inline void*					GetHostVisibleData() { return m_HostVisibleData; }
	inline VkBuffer					GetBuffer() { return m_Buffer; }
	uint32_t						GetID() { return m_Id; }
	inline uint64_t					GetSize() { return m_Size; }

private:
	void CreateBufferResource();
	
private:
	VulkanDevice&			m_Device;
	VulkanBufferInfo		m_Info{};
	VkBuffer				m_Buffer;
	VmaAllocation			m_VmaAllocation;
	void*					m_HostVisibleData = nullptr;

	std::string				m_Name;
	uint64_t				m_Size;
};