#include "precomp.h"

#include "VulkanTypes.h"

#include <iostream>
#include <chrono>
#include <ctime>

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
namespace Utils
{
	long long SetStartTime()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}

	void SetEndtimeAndPrint(long long start)
	{
		auto endtime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		std::cout << endtime - start << "ms" << std::endl;
	}
}

