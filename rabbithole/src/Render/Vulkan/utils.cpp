#include "precomp.h"

#include "VulkanTypes.h"
#include <iostream>

AllocationTrack s_Allocations;

void PrintUsage()
{
#ifdef USE_MEMORY_TRACKER
	std::cout << "Current Usage: " << s_Allocations.CurrentUsage() << std::endl;
#endif
}

AllocationTrack GetAllocationTrack()
{
	return s_Allocations;
}

#ifdef USE_MEMORY_TRACKER
void* operator new (size_t size)
{
	s_Allocations.TotalAllocated += size;
	return malloc(size);
}

void operator delete(void* memory, size_t size)
{
	s_Allocations.TotalFreed += size;
	free(memory);
}
#endif