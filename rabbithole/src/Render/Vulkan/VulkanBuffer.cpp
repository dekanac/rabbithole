#include "precomp.h"

#include "vk_mem_alloc.h"

uint32_t VulkanBuffer::ms_currentId = BUFFER_ID_START_POS;

VulkanBuffer::VulkanBuffer(const VulkanDevice* device, const VulkanBufferInfo& info)
	: m_Device(device)
	, m_Info(info)
{
	CreateBufferResource();
}

VulkanBuffer::VulkanBuffer(const VulkanDevice* device, BufferUsageFlags flags, MemoryAccess access, uint64_t size)
	: m_Device(device)
	, m_Size(size)
{
	VulkanBufferInfo info{};
	info.memoryAccess = access;
	info.size = size;
	info.usageFlags = flags;
	m_Info = info;
	CreateBufferResource();
}


VulkanBuffer::~VulkanBuffer()
{
	vmaDestroyBuffer(m_Device->GetVmaAllocator(), m_Buffer, m_VmaAllocation);
}

void* VulkanBuffer::Map()
{
	ASSERT(m_HostVisibleData == nullptr, "Host Visible data pointer is pointing to something!!!");

	if (m_Info.memoryAccess == MemoryAccess::Host)
	{
		vmaMapMemory(m_Device->GetVmaAllocator(), m_VmaAllocation, &m_HostVisibleData);
	}

	return m_HostVisibleData;
}

void VulkanBuffer::Unmap()
{
	if (m_HostVisibleData)
	{
		vmaUnmapMemory(m_Device->GetVmaAllocator(), m_VmaAllocation);
		m_HostVisibleData = nullptr;
	}
}

void VulkanBuffer::FillBuffer(void* inputData, size_t size)
{
	void* data = Map();
	memcpy(data, inputData, size);
	Unmap();
}

void VulkanBuffer::CreateBufferResource()
{
	m_Id = ms_currentId;
	ms_currentId++; 
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = m_Info.size;
	bufferInfo.usage = GetVkBufferUsageFlags(m_Info.usageFlags);
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocationCreateInfo = {};
	allocationCreateInfo.usage = GetVmaMemoryUsageFrom(m_Info.memoryAccess);

	vmaCreateBuffer(m_Device->GetVmaAllocator(), &bufferInfo, &allocationCreateInfo, &m_Buffer, &m_VmaAllocation, nullptr);
}
