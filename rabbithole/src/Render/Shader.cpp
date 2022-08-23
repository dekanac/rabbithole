#include "common.h"

#include "Render/Converters.h"
#include "Shader.h"
#include "Logger/Logger.h"

#include "Render/Vulkan/VulkanDevice.h"
#include "Render/Vulkan/spirv-reflect/spirv_reflect.h"

#include <vulkan/vulkan.h>
#include <crc32/Crc32.h>

Shader::Shader(VulkanDevice& device, size_t byteCodeSize, const char* byteCode, const ShaderInfo& info, const char* name)
	: m_Device(device)
	, m_Info(info)
	, m_Name(name)
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

	uint32_t pushConstsCount = 0;
	VULKAN_SPIRV_CALL(spvReflectEnumeratePushConstantBlocks(&spvModule, &pushConstsCount, nullptr));

	std::vector<SpvReflectBlockVariable*> pushConstants(pushConstsCount);
	VULKAN_SPIRV_CALL(spvReflectEnumeratePushConstantBlocks(&spvModule, &pushConstsCount, pushConstants.data()));

	m_PushConstants.resize(pushConstsCount);

	for (uint32_t i = 0; i < pushConstsCount; i++)
	{
		VkPushConstantRange& pushConstantRange = m_PushConstants[i];
		pushConstantRange.stageFlags = GetVkShaderStageFrom(m_Info.Type);
		pushConstantRange.offset = pushConstants[i]->offset;
		pushConstantRange.size = pushConstants[i]->size;
	}

	spvReflectDestroyShaderModule(&spvModule);

	m_Hash = crc32_fast((const void*)byteCode, byteCodeSize);
}

Shader::~Shader()
{
	vkDestroyShaderModule(m_Device.GetGraphicDevice(), m_ShaderModule, nullptr);
}
