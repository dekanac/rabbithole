#pragma once

#include <vector>
#include <string>
#include <vulkan/vulkan.h>

#include "Vulkan/VulkanTypes.h"

class VulkanDevice;

struct ShaderInfo
{
	ShaderType Type;
	const char* CodeEntry;
};

class Shader
{
public:
	Shader(VulkanDevice& device,
		size_t byteCodeSize,
		const char* byteCode,
		const ShaderInfo& info);
	~Shader();

public:
	inline const ShaderInfo GetInfo() const { return m_Info; }
	inline const VkShaderModule GetModule()  { return m_ShaderModule; }

private:
	const ShaderInfo m_Info;
	
	VkShaderModule m_ShaderModule;
	std::vector<VkDescriptorSetLayoutBinding> m_DescriptorSetLayoutBindings;
	VulkanDevice& m_Device;

	const char* m_Name;

};