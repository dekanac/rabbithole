#include "common.h"

#include "Shader.h"
#include "Logger/Logger.h"

#include "Vulkan/VulkanDevice.h"
#include "Vulkan/spirv-reflect/spirv_reflect.h"


Shader::Shader(VulkanDevice& device, size_t byteCodeSize, const char* byteCode, const ShaderInfo& info)
	: m_Device(device)
	, m_Info(info)
{
	VkShaderModuleCreateInfo shaderCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	shaderCreateInfo.codeSize = byteCodeSize;
	shaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(byteCode);

	VULKAN_API_CALL(vkCreateShaderModule(m_Device.GetGraphicDevice(), &shaderCreateInfo, nullptr, &m_ShaderModule));

	SpvReflectShaderModule spvModule;
	VULKAN_SPIRV_CALL(spvReflectCreateShaderModule(byteCodeSize, byteCode, &spvModule));

	uint32_t bindingCount = 0;
	VULKAN_SPIRV_CALL(spvReflectEnumerateDescriptorBindings(&spvModule, &bindingCount, nullptr));

	std::vector<SpvReflectDescriptorBinding*> bindings(bindingCount);
	VULKAN_SPIRV_CALL(spvReflectEnumerateDescriptorBindings(&spvModule, &bindingCount, bindings.data()));

	m_DescriptorSetLayoutBindings.resize(bindingCount);
	for (uint32_t i = 0; i < bindingCount; ++i)
	{
		VkDescriptorSetLayoutBinding& descriptorSetLayoutBinding = m_DescriptorSetLayoutBindings[i];
		descriptorSetLayoutBinding.binding = bindings[i]->binding;
		descriptorSetLayoutBinding.descriptorCount = 1;
		descriptorSetLayoutBinding.descriptorType = GetVkDescriptorTypeFrom(bindings[i]->descriptor_type);
		descriptorSetLayoutBinding.stageFlags = GetVkShaderStageFrom(m_Info.Type);
		descriptorSetLayoutBinding.pImmutableSamplers = nullptr;
	}

	spvReflectDestroyShaderModule(&spvModule);
}

Shader::~Shader()
{
	vkDestroyShaderModule(m_Device.GetGraphicDevice(), m_ShaderModule, nullptr);
}
