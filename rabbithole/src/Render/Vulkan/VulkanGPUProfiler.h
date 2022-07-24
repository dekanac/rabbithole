#pragma once

#include <vector>
#include <string>

class VulkanDevice;

struct TimeStamp
{
	std::string label;
	float       microseconds;
};

class GPUTimeStamps
{
public:
	void OnCreate(VulkanDevice* device, uint32_t numberOfBackBuffers);
	void OnDestroy();

	void GetTimeStamp(VkCommandBuffer cmd_buf, const char* label);
	void GetTimeStampUser(TimeStamp ts);
	void OnBeginFrame(VkCommandBuffer cmd_buf, std::vector<TimeStamp>* pTimestamp);
	void OnEndFrame();

private:
	VulkanDevice* m_Device;

	uint32_t m_NumberOfBackBuffers;
	uint32_t m_Frame;

	VkQueryPool	m_QueryPool;

	const uint32_t MaxValuesPerFrame = 128;

	std::vector<std::string> m_labels[5];
	std::vector<TimeStamp> m_cpuTimeStamps[5];
};