#include "precomp.h"

#include "vk_mem_alloc.h"
#include "Render/Converters.h"

VulkanBuffer::VulkanBuffer(VulkanDevice& device, BufferCreateInfo createInfo)
	: ManagableResource(ResourceType::Buffer)
	, m_Device(device)
	, m_Info(createInfo)
{
	if (m_Info.memoryAccess == MemoryAccess::GPU)
	{
		//if the access is only gpu we need to copy from staging buffer
		m_Info.flags = m_Info.flags | BufferUsageFlags::TransferDst;
	}
	
	CreateBufferResource();

	device.SetObjectName((uint64_t)m_Buffer, VK_OBJECT_TYPE_BUFFER, createInfo.name.c_str());

	m_CurrentResourceState = ResourceState::None;
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

void VulkanBuffer::FillBuffer(void* inputData, uint64_t size, uint64_t offset)
{
	ASSERT(offset + size <= m_Info.size, "Trying to reach outside buffer's bounds!");

	if (size > 0)
	{
		if (m_Info.memoryAccess != MemoryAccess::GPU)
		{
			void* data = Map();
			memcpy(data, (char*)inputData + offset, size);
			Unmap();
		}
		else
		{
			VulkanBuffer stagingBuffer(m_Device, BufferCreateInfo{
					.flags = BufferUsageFlags::TransferSrc,
					.memoryAccess = MemoryAccess::CPU,
					.size = size,
					.name = "StagingBuffer"
				});

			stagingBuffer.FillBuffer(inputData);

			//TODO: no way! Temp Command Buffers should be used only in init phases!!!
			VulkanCommandBuffer tempCommandBuffer(m_Device, "Temp command buffer!");
			tempCommandBuffer.BeginCommandBuffer();

			m_Device.CopyBuffer(tempCommandBuffer, stagingBuffer, *this, size, 0, offset);

			tempCommandBuffer.EndAndSubmitCommandBuffer();
		}
	}
}

void VulkanBuffer::FillBuffer(void* data)
{
	FillBuffer(data, GetSize(), 0);
}

void VulkanBuffer::CreateBufferResource()
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = m_Info.size;
	bufferInfo.usage = GetVkBufferUsageFlags(m_Info.flags);
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocationCreateInfo = {};
	allocationCreateInfo.usage = GetVmaMemoryUsageFrom(m_Info.memoryAccess);

	vmaCreateBuffer(m_Device.GetVmaAllocator(), &bufferInfo, &allocationCreateInfo, &m_Buffer, &m_VmaAllocation, nullptr);
}
