#include "precomp.h"

void GPUTimeStamps::OnCreate(VulkanDevice* device, uint32_t numberOfBackBuffers)
{
	m_Device = device;
	m_NumberOfBackBuffers = numberOfBackBuffers;
	m_Frame = 0;

	const VkQueryPoolCreateInfo queryPoolCreateInfo =
	{
		VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,     // VkStructureType                  sType
		NULL,                                         // const void*                      pNext
		(VkQueryPoolCreateFlags)0,                    // VkQueryPoolCreateFlags           flags
		VK_QUERY_TYPE_TIMESTAMP ,                     // VkQueryType                      queryType
		MaxValuesPerFrame * numberOfBackBuffers,      // deUint32                         entryCount
		0,                                            // VkQueryPipelineStatisticFlags    pipelineStatistics
	};

	VULKAN_API_CALL(vkCreateQueryPool(m_Device->GetGraphicDevice(), &queryPoolCreateInfo, NULL, &m_QueryPool));
}

void GPUTimeStamps::OnDestroy()
{
	vkDestroyQueryPool(m_Device->GetGraphicDevice(), m_QueryPool, nullptr);

	for (uint32_t i = 0; i < m_NumberOfBackBuffers; i++)
		m_labels[i].clear();
}

void GPUTimeStamps::GetTimeStamp(VulkanCommandBuffer& cmd_buf, const char* label)
{
	uint32_t measurements = (uint32_t)m_labels[m_Frame].size();
	uint32_t offset = m_Frame * MaxValuesPerFrame + measurements;

	vkCmdWriteTimestamp(GET_VK_HANDLE(cmd_buf), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_QueryPool, offset);

	m_labels[m_Frame].push_back(label);
}

void GPUTimeStamps::OnBeginFrame(VulkanCommandBuffer& cmd_buf, std::vector<TimeStamp>* pTimestamps)
{
	std::vector<TimeStamp>& cpuTimeStamps = m_cpuTimeStamps[m_Frame];
	std::vector<std::string>& gpuLabels = m_labels[m_Frame];

	pTimestamps->clear();
	pTimestamps->reserve(cpuTimeStamps.size() + gpuLabels.size());

	// copy CPU timestamps
	//
	for (uint32_t i = 0; i < cpuTimeStamps.size(); i++)
	{
		pTimestamps->push_back(cpuTimeStamps[i]);
	}

	// copy GPU timestamps
	//
	uint32_t offset = m_Frame * MaxValuesPerFrame;

	uint32_t measurements = (uint32_t)gpuLabels.size();
	if (measurements > 0)
	{
		// timestampPeriod is the number of nanoseconds per timestamp value increment
		double microsecondsPerTick = (1e-3f * m_Device->GetPhysicalDeviceProperties().properties.limits.timestampPeriod);
		{
			UINT64 TimingsInTicks[256] = {};
			VULKAN_API_CALL(vkGetQueryPoolResults(m_Device->GetGraphicDevice(), m_QueryPool, offset, measurements, measurements * sizeof(UINT64), &TimingsInTicks, sizeof(UINT64), VK_QUERY_RESULT_64_BIT));

			for (uint32_t i = 1; i < measurements; i++)
			{
				TimeStamp ts = { m_labels[m_Frame][i], float(microsecondsPerTick * (double)(TimingsInTicks[i] - TimingsInTicks[i - 1])) };
				pTimestamps->push_back(ts);
			}

			// compute total
			TimeStamp ts = { "Total GPU Time", float(microsecondsPerTick * (double)(TimingsInTicks[measurements - 1] - TimingsInTicks[0])) };
			pTimestamps->push_back(ts);
		}
	}

	vkCmdResetQueryPool(GET_VK_HANDLE(cmd_buf), m_QueryPool, offset, MaxValuesPerFrame);

	// we always need to clear these ones
	cpuTimeStamps.clear();
	gpuLabels.clear();

	GetTimeStamp(cmd_buf, "Begin Frame");
}

void GPUTimeStamps::OnEndFrame()
{
	m_Frame = (m_Frame + 1) % m_NumberOfBackBuffers;
}
