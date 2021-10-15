#pragma once

#include <vector>
#include <string>
#include <vulkan/vulkan.h>

#include "VulkanTypes.h"

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
		const ShaderInfo& info,
		const char* name);
	~Shader();

public:
	inline const ShaderInfo GetInfo() const { return m_Info; }
	inline const VkShaderModule GetModule()  { return m_ShaderModule; }
	inline const char* GetName() const { return m_Name; }
	inline const uint32_t GetHash() const { return m_Hash; }
	inline const std::vector<VkDescriptorSetLayoutBinding>& GetDescriptorSetLayoutBindings() const { return m_DescriptorSetLayoutBindings; }

private:
	const ShaderInfo m_Info;
	
	VkShaderModule m_ShaderModule;
	std::vector<VkDescriptorSetLayoutBinding> m_DescriptorSetLayoutBindings;
	VulkanDevice& m_Device;

	const char* m_Name;
	//calculate the hash fucntion, this will be used in pipeline key
	uint32_t m_Hash;

};