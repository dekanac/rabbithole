#include "precomp.h"

#include "vk_mem_alloc.h"
#include "Render/Converters.h"

VulkanBuffer::VulkanBuffer(VulkanDevice& device, BufferUsageFlags flags, MemoryAccess access, uint64_t size, const char* name)
	: m_Name(name)
	, m_Device(device)
	, m_Size(size)
{
	m_Info.memoryAccess = access;
	m_Info.size = size;
	m_Info.usageFlags = flags;

	if (m_Info.memoryAccess == MemoryAccess::GPU)
	{
		//if the access is only gpu we need to copy from staging buffer
		m_Info.usageFlags = m_Info.usageFlags | BufferUsageFlags::TransferDst;
	}

	CreateBufferResource();

	device.SetObjectName((uint64_t)m_Buffer, VK_OBJECT_TYPE_BUFFER, name);
}


VulkanBuffer::VulkanBuffer(VulkanDevice& device, BufferCreateInfo& createInfo)
	: m_Name(createInfo.name)
	, m_Device(device)
	, m_Size(createInfo.size)
{
	m_Info.memoryAccess = createInfo.memoryAccess;
	m_Info.size = createInfo.size;
	m_Info.usageFlags = createInfo.flags;

	if (m_Info.memoryAccess == MemoryAccess::GPU)
	{
		//if the access is only gpu we need to copy from staging buffer
		m_Info.usageFlags = m_Info.usageFlags | BufferUsageFlags::TransferDst;
	}
	
	CreateBufferResource();

	device.SetObjectName((uint64_t)m_Buffer, VK_OBJECT_TYPE_BUFFER, createInfo.name.c_str());
}

VulkanBuffer::~VulkanBuffer()
{
	Unmap();
	vmaDestroyBuffer(m_Device.GetVmaAllocator(), m_Buffer, m_VmaAllocation);
}

void* VulkanBuffer::Map()
{
	ASSERT(m_HostVisibleData == nullptr, "Host Visible data pointer is pointing to something!!!");

	if (m_Info.memoryAccess == MemoryAccess::CPU || m_Info.memoryAccess == MemoryAccess::CPU2GPU)
	{
		vmaMapMemory(m_Device.GetVmaAllocator(), m_VmaAllocation, &m_HostVisibleData);
	}

	return m_HostVisibleData;
}

void VulkanBuffer::Unmap()
{
	if (m_HostVisibleData)
	{
		vmaUnmapMemory(m_Device.GetVmaAllocator(), m_VmaAllocation);
		m_HostVisibleData = nullptr;
	}
}

//TODO: fix this, move code to OFFSET version and then from here call FillBuffer(data, 0, size)
void VulkanBuffer::FillBuffer(void* inputData, uint64_t size)
{
	if (m_Info.memoryAccess != MemoryAccess::GPU)
	{
		void* data = Map();
		memcpy(data, inputData, size);
		Unmap();
	}
	else
	{
		VulkanBuffer stagingBuffer = VulkanBuffer(m_Device, BufferUsageFlags::TransferSrc, MemoryAccess::CPU, size, "StagingBuffer");

		stagingBuffer.FillBuffer(inputData, static_cast<size_t>(size));

		m_Device.CopyBuffer(stagingBuffer.GetBuffer(), m_Buffer, size);
	}
}

void VulkanBuffer::FillBuffer(void* inputData, uint64_t offset, uint64_t size)
{
	void* data = Map();
	char* dataOffset = (char*)data + offset;
	memcpy(dataOffset, inputData, size);
	Unmap();
}

void VulkanBuffer::FillBuffer(void* data)
{
	FillBuffer(data, GetSize());
}

void VulkanBuffer::CreateBufferResource()
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = m_Info.size;
	bufferInfo.usage = GetVkBufferUsageFlags(m_Info.usageFlags);
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocationCreateInfo = {};
	allocationCreateInfo.usage = GetVmaMemoryUsageFrom(m_Info.memoryAccess);

	vmaCreateBuffer(m_Device.GetVmaAllocator(), &bufferInfo, &allocationCreateInfo, &m_Buffer, &m_VmaAllocation, nullptr);
}
