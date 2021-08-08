#include "precomp.h"

#include "vk_mem_alloc.h"

VulkanBuffer::VulkanBuffer(const VulkanDevice* device, const VulkanBufferInfo& info)
	: m_Device(device)
	, m_Info(info)
{
	CreateBufferResource();
}

VulkanBuffer::VulkanBuffer(const VulkanDevice* device, BufferUsageFlags flags, MemoryAccess access, uint64_t size)
	: m_Device(device)
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

void VulkanBuffer::CreateBufferResource()
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = m_Info.size;
	bufferInfo.usage = GetVkBufferUsageFlags(m_Info.usageFlags);
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocationCreateInfo = {};
	allocationCreateInfo.usage = GetVmaMemoryUsageFrom(m_Info.memoryAccess);

	vmaCreateBuffer(m_Device->GetVmaAllocator(), &bufferInfo, &allocationCreateInfo, &m_Buffer, &m_VmaAllocation, nullptr);
}

WrappedBuffer::WrappedBuffer(const VulkanDevice* device, BufferUsageFlags bufferType, uint32_t size, void* data)
	: m_VulkanDevice(device)
{

	memcpy(static_cast<char*>(ms_Data) + ms_CurrentOffset, data, size);

	m_Offset = ms_CurrentOffset;
	m_Size = size;

	ms_CurrentOffset += size;
}

void WrappedBuffer::InitializeBuffer(VulkanDevice* device)
{
	ms_Data = malloc(134217728);
	ms_CurrentOffset = 0;
	ms_MainGPUBuffer = nullptr;
}

VulkanBuffer*	WrappedBuffer::ms_MainGPUBuffer = nullptr;
uint32_t		WrappedBuffer::ms_CurrentOffset = 0;
void*			WrappedBuffer::ms_Data = nullptr;

void WrappedBuffer::BindBuffer(VulkanDevice* device)
{
	VulkanBuffer* stagingBuffer = new VulkanBuffer(device, BufferUsageFlags::TransferSrc, MemoryAccess::Host, ms_CurrentOffset);
	
	stagingBuffer->Map();
	memcpy(stagingBuffer->GetHostVisibleData(), ms_Data, static_cast<size_t>(ms_CurrentOffset));
	stagingBuffer->Unmap();
	
	ms_MainGPUBuffer = new VulkanBuffer(device, BufferUsageFlags::VertexBuffer | BufferUsageFlags::TransferDst, MemoryAccess::Device, ms_CurrentOffset);

	device->CopyBuffer(stagingBuffer->GetBuffer(), ms_MainGPUBuffer->GetBuffer(), ms_CurrentOffset);

	delete stagingBuffer;
}

WrappedBuffer::~WrappedBuffer()
{
}