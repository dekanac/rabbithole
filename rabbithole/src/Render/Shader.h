#pragma once

#include <vector>
#include <string>
#include <vulkan/vulkan.h>

#include "Render/Vulkan/VulkanTypes.h"
#include "Render/Resource.h"

class VulkanDevice;

struct ShaderInfo
{
	ShaderType Type;
	const char* CodeEntry;
};

class Shader : public AllocatedResource
{
public:
	friend class ResourceManager; // Resource Manager will take care of creation and deletion of shaders

private:
	Shader(VulkanDevice& device,
		size_t byteCodeSize,
		const char* byteCode,
		const ShaderInfo& info,
		const char* name);
	~Shader();

public:
	inline const ShaderInfo		GetInfo() const { return m_Info; }
	inline const VkShaderModule GetModule()  { return m_ShaderModule; }
	inline const char*			GetName() const { return m_Name; }
	inline const uint32_t		GetHash() const { return m_Hash; }
	inline const ShaderType		GetType() const { return m_Info.Type; }
	inline const std::vector<VkDescriptorSetLayoutBinding>& GetDescriptorSetLayoutBindings() const { return m_DescriptorSetLayoutBindings; }
	inline const std::vector<VkPushConstantRange>& GetPushConstants() const { return m_PushConstants; }

private:
	const ShaderInfo m_Info;
	
	VulkanDevice& m_Device;
	VkShaderModule m_ShaderModule;
	std::vector<VkDescriptorSetLayoutBinding> m_DescriptorSetLayoutBindings;
	std::vector<VkPushConstantRange> m_PushConstants;

	const char* m_Name;
	//calculate the hash function, this will be used in pipeline key
	uint32_t m_Hash;
};