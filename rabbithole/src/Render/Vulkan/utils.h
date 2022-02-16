#pragma once

#ifdef _DEBUG
	#define USE_MEMORY_TRACKER
#endif

struct AllocationTrack
{
	uint32_t TotalAllocated = 0;
	uint32_t TotalFreed = 0;

	uint32_t CurrentUsage() { return TotalAllocated - TotalFreed; }
};

void PrintUsage();
AllocationTrack GetAllocationTrack();