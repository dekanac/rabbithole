#include "Render/Vulkan/VulkanTypes.h"
#include "Render/Vulkan/spirv-reflect/spirv_reflect.h"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

VkBufferUsageFlags			GetVkBufferUsageFlags(const BufferUsageFlags usageFlags);
VmaMemoryUsage				GetVmaMemoryUsageFrom(const MemoryAccess memoryAccess);
VkDescriptorType			GetVkDescriptorTypeFrom(const DescriptorType descriptorSetBinding);
VkDescriptorType			GetVkDescriptorTypeFrom(const SpvReflectDescriptorType reflectDescriptorType);
VkShaderStageFlagBits		GetVkShaderStageFrom(const ShaderType shaderType);
VkShaderStageFlagBits		GetVkShaderStageFrom(const PipelineType pipelineType);
ResourceStage				GetResourceStageFrom(const PipelineType pipelineType);
VkFormat					GetVkFormatFrom(const Format format);
VkColorSpaceKHR				GetVkColorSpaceFrom(const ColorSpace colorSpace);
uint32_t					GetBlockSizeFrom(const VkFormat format);
VkImageType					GetVkImageTypeFrom(const uint32_t imageDepth);
VkImageAspectFlags			GetVkImageAspectFlagsFrom(const VkFormat format);
VkImageLayout				GetVkImageLayoutFrom(const ResourceState resourceState);
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
ClearValue					GetClearColorValueFor(const Format format);
uint32_t					GetBPPFrom(const Format format);
VkImageUsageFlags			GetVkImageUsageFlagsFrom(const ImageUsageFlags usageFlags);
VkBorderColor				GetVkBorderColorFrom(const Color color);
bool						IsDepthFormat(const Format format);
VkPipelineStageFlags		GetVkPipelineStageFromResourceStageAndState(const ResourceStage stage, const ResourceState state, bool isSrcStage);
VkAccessFlags				GetVkAccessFlagsFromResourceState(const ResourceState state);
enum VkPipelineBindPoint	GetVkBindPointFrom(const PipelineType pipelineType);
VkAttachmentLoadOp			GetVkLoadOpFrom(LoadOp op);
uint64_t					GetTextureSizeFrom(Format format, Extent3D textureExtent, uint32_t mipCount, uint32_t arrayCount);
uint64_t					GetTexelSizeFrom(Format format);
ShaderType					GetShaderStageFrom(const std::string& name);