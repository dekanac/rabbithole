#include "Render/Vulkan/VulkanTypes.h"
#include "Render/Vulkan/spirv-reflect/spirv_reflect.h"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

VkBufferUsageFlags			GetVkBufferUsageFlags(const BufferUsageFlags usageFlags);
VmaMemoryUsage				GetVmaMemoryUsageFrom(const MemoryAccess memoryAccess);
VkDescriptorType			GetVkDescriptorTypeFrom(const DescriptorType descriptorSetBinding);
VkDescriptorType			GetVkDescriptorTypeFrom(const SpvReflectDescriptorType reflectDescriptorType);
VkShaderStageFlagBits		GetVkShaderStageFrom(const ShaderType shaderType);
VkFormat					GetVkFormatFrom(const Format format);
VkColorSpaceKHR				GetVkColorSpaceFrom(const ColorSpace colorSpace);
uint32_t					GetBlockSizeFrom(const VkFormat format);
VkImageType					GetVkImageTypeFrom(const uint32_t imageDepth);
VkImageAspectFlags			GetVkImageAspectFlagsFrom(const VkFormat format);
VkImageLayout				GetVkImageLayoutFrom(const ResourceState resourceState);
VkAccessFlags				GetVkAccessFlagsFrom(const ResourceState resourceState);
VkPipelineStageFlagBits		GetGraphicsPipelineStage(const ResourceState resourceState);
VkPipelineStageFlagBits		GetTransferPipelineStage(const ResourceState resourceState);
VkPrimitiveTopology			GetVkPrimitiveTopologyFrom(const Topology topology);
VkSampleCountFlagBits		GetVkSampleFlagsFrom(const MultisampleType multiSampleType);
VkCompareOp					GetVkCompareOperationFrom(const CompareOperation compareOperation);
VkStencilOp					GetVkStencilOpFrom(const StencilOperation stencilOperation);
VkFilter					GetVkFilterFrom(const FilterType filterType);
VkSamplerMipmapMode			GetVkMipmapModeFrom(const FilterType filterType);
VkSamplerAddressMode		GetVkAddressModeFrom(const AddressMode addressMode);
VkBlendFactor				GetVkBlendFactorFrom(const BlendValue blendValue);
VkBlendOp					GetVkBlendOpFrom(const BlendOperation blendOperation);
VkVertexInputRate			GetVkVertexInputRateFrom(const VertexInputRate inputRate);
VkClearValue				GetVkClearColorValueFor(const Format format);
ClearValue					GetClearColorValueFor(const Format format);
uint32_t					GetBPPFrom(const Format format);
VkImageUsageFlags			GetVkImageUsageFlagsFrom(const ImageUsageFlags usageFlags);
VkBorderColor				GetVkBorderColorFrom(const Color color);
bool						IsDepthFormat(const Format format);
VkPipelineStageFlags		GetVkPipelineStageFromResourceStageAndState(const ResourceStage stage, const ResourceState state, bool isSrcStage);
VkAccessFlags				GetVkAccessFlagsFromResourceState(const ResourceState state);
enum VkPipelineBindPoint	GetVkBindPointFrom(const PipelineType pipelineType);