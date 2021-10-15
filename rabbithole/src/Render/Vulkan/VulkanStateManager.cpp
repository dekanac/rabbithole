#include "precomp.h"

VulkanStateManager::VulkanStateManager()
{
	m_Pipeline = nullptr;
	m_RenderPass = nullptr;
}

VulkanStateManager::~VulkanStateManager()
{
	delete m_RenderPass;
	delete m_Pipeline;
}
