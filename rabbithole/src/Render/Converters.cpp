#include "Converters.h"

#include "Logger/Logger.h"

VkBufferUsageFlags GetVkBufferUsageFlags(const BufferUsageFlags usageFlags)
{
	VkBufferUsageFlags bufferUsageFlags = 0;
	if (IsFlagSet(usageFlags & BufferUsageFlags::TransferSrc))
	{
		bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}

	if (IsFlagSet(usageFlags & BufferUsageFlags::TransferDst))
	{
		bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}

	if (IsFlagSet(usageFlags & BufferUsageFlags::ResrcBuffer))
	{
		bufferUsageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		bufferUsageFlags |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
	}

	if (IsFlagSet(usageFlags & BufferUsageFlags::StorageBuffer))
	{
		bufferUsageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		bufferUsageFlags |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
	}

	if (IsFlagSet(usageFlags & BufferUsageFlags::VertexBuffer))
	{
		bufferUsageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	}

	if (IsFlagSet(usageFlags & BufferUsageFlags::IndexBuffer))
	{
		bufferUsageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	}

	if (IsFlagSet(usageFlags & BufferUsageFlags::UniformBuffer))
	{
		bufferUsageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	}

	if (IsFlagSet(usageFlags & BufferUsageFlags::IndirectBuffer))
	{
		bufferUsageFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	}

	return bufferUsageFlags;
}

VmaMemoryUsage GetVmaMemoryUsageFrom(const MemoryAccess memoryAccess)
{
	switch (memoryAccess)
	{
	case MemoryAccess::CPU:
		return VMA_MEMORY_USAGE_CPU_ONLY;
	case MemoryAccess::GPU:
		return VMA_MEMORY_USAGE_GPU_ONLY;
	case MemoryAccess::CPU2GPU:
		return VMA_MEMORY_USAGE_CPU_TO_GPU;
	default:
		ASSERT(false, "Not supported MemoryAccessFlags.");
		return VMA_MEMORY_USAGE_UNKNOWN;
	}
}

VkDescriptorType GetVkDescriptorTypeFrom(const DescriptorType descriptorSetBinding)
{
	switch (descriptorSetBinding)
	{
	case DescriptorType::CombinedSampler:
		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	case DescriptorType::UniformBuffer:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case DescriptorType::StorageImage:
		return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case DescriptorType::StorageBuffer:
		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	case DescriptorType::SampledImage:
		return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case DescriptorType::Sampler:
		return VK_DESCRIPTOR_TYPE_SAMPLER;
	default:
		ASSERT(false, "Not supported DescriptorSetBindingType.");
		return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
}

VkShaderStageFlagBits GetVkShaderStageFrom(const ShaderType shaderType)
{
	switch (shaderType)
	{
	case ShaderType::Vertex:
		return VK_SHADER_STAGE_VERTEX_BIT;
	case ShaderType::Fragment:
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	case ShaderType::Geometry:
		return VK_SHADER_STAGE_GEOMETRY_BIT;
	case ShaderType::Hull:
		return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	case ShaderType::Domain:
		return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	case ShaderType::Compute:
		return VK_SHADER_STAGE_COMPUTE_BIT;
	default:
		ASSERT(false, "Not supported ShaderType.");
		return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
	}
}

VkDescriptorType GetVkDescriptorTypeFrom(const SpvReflectDescriptorType reflectDescriptorType)
{
	switch (reflectDescriptorType)
	{
	case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
		return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
		return VK_DESCRIPTOR_TYPE_SAMPLER;
	default:
		ASSERT(false, "Not supported SpvReflectDescriptorBinding.");
		return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
}

VkFormat GetVkFormatFrom(const Format format)
{
	switch (format)
	{
	case Format::D32_SFLOAT:
		return VK_FORMAT_D32_SFLOAT;
	case Format::D32_SFLOAT_S8_UINT:
		return VK_FORMAT_D32_SFLOAT_S8_UINT;
	case Format::B8G8R8A8_UNORM:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case Format::B8G8R8A8_UNORM_SRGB:
		return VK_FORMAT_B8G8R8A8_SRGB;
	case Format::R8G8B8A8_UNORM:
		return VK_FORMAT_R8G8B8A8_UNORM;
	case Format::R8G8B8A8_UNORM_SRGB:
		return VK_FORMAT_R8G8B8A8_SRGB;
	case Format::R32_SFLOAT:
		return VK_FORMAT_R32_SFLOAT;
	case Format::R8_UNORM:
		return VK_FORMAT_R8_UNORM;
	case Format::R8_UINT:
		return VK_FORMAT_R8_UINT;
	case Format::R16G16B16A16_FLOAT:
		return VK_FORMAT_R16G16B16A16_SFLOAT;
	case Format::R16G16B16A16_UNORM:
		return VK_FORMAT_R16G16B16A16_UNORM;
	case Format::R32G32B32A32_FLOAT:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	case Format::R32G32B32_FLOAT:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case Format::R11G11B10_FLOAT:
		return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
	case Format::R32G32_FLOAT:
		return VK_FORMAT_R32G32_SFLOAT;
	case Format::R16G16_FLOAT:
		return VK_FORMAT_R16G16_SFLOAT;
	case Format::BC1_UNORM:
		return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
	case Format::BC1_UNORM_SRGB:
		return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
	case Format::BC2_UNORM:
		return VK_FORMAT_BC2_UNORM_BLOCK;
	case Format::BC2_UNORM_SRGB:
		return VK_FORMAT_BC2_SRGB_BLOCK;
	case Format::BC3_UNORM:
		return VK_FORMAT_BC3_UNORM_BLOCK;
	case Format::BC3_UNORM_SRGB:
		return VK_FORMAT_BC3_SRGB_BLOCK;
	case Format::BC7_UNORM:
		return VK_FORMAT_BC7_UNORM_BLOCK;
	case Format::BC7_UNORM_SRGB:
		return VK_FORMAT_BC7_SRGB_BLOCK;
	case Format::R32_UINT:
		return VK_FORMAT_R32_UINT;
	default:
		ASSERT(false, "Not supported Format.");
		return VK_FORMAT_UNDEFINED;
	}
}

VkColorSpaceKHR GetVkColorSpaceFrom(const ColorSpace colorSpace)
{
	switch (colorSpace)
	{
	case ColorSpace::SRGB:
		return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	case ColorSpace::ExtendedSRGB_Linear:
		return VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT;
	case ColorSpace::ExtendedSRGB_Nonlinear:
		return VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT;
	default:
		ASSERT(false, "Not supported ColorSpace.");
		return VK_COLOR_SPACE_MAX_ENUM_KHR;
	}
}

uint32_t GetBlockSizeFrom(const VkFormat format)
{
	if ((format == VK_FORMAT_BC1_RGBA_UNORM_BLOCK) ||
		(format == VK_FORMAT_BC1_RGBA_SRGB_BLOCK) ||
		(format == VK_FORMAT_BC2_UNORM_BLOCK) ||
		(format == VK_FORMAT_BC2_SRGB_BLOCK) ||
		(format == VK_FORMAT_BC3_UNORM_BLOCK) ||
		(format == VK_FORMAT_BC3_SRGB_BLOCK) ||
		(format == VK_FORMAT_BC4_UNORM_BLOCK) ||
		(format == VK_FORMAT_BC5_UNORM_BLOCK) ||
		(format == VK_FORMAT_BC6H_UFLOAT_BLOCK) ||
		(format == VK_FORMAT_BC7_UNORM_BLOCK) ||
		(format == VK_FORMAT_BC7_SRGB_BLOCK))
	{
		return 4;
	}
	else
	{
		return 1;
	}
}

VkImageUsageFlags GetVkImageUsageFlagsFrom(const ImageUsageFlags usageFlags)
{
	VkImageUsageFlags imageUsageFlags = 0;
	if (IsFlagSet(usageFlags & ImageUsageFlags::TransferSrc))
	{
		imageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	if (IsFlagSet(usageFlags & ImageUsageFlags::TransferDst))
	{
		imageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	if (IsFlagSet(usageFlags & ImageUsageFlags::Resource))
	{
		imageUsageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	}

	if (IsFlagSet(usageFlags & ImageUsageFlags::Storage))
	{
		imageUsageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
	}

	if (IsFlagSet(usageFlags & ImageUsageFlags::RenderTarget))
	{
		imageUsageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	if (IsFlagSet(usageFlags & ImageUsageFlags::DepthStencil))
	{
		imageUsageFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}

	return imageUsageFlags;
}

VkImageType GetVkImageTypeFrom(const uint32_t imageDepth)
{
	if (imageDepth > 1)
	{
		return VK_IMAGE_TYPE_3D;
	}
	else
	{
		return VK_IMAGE_TYPE_2D;
	}
}

VkImageAspectFlags GetVkImageAspectFlagsFrom(VkFormat format)
{
	switch (format)
	{
	case VK_FORMAT_D16_UNORM:
	case VK_FORMAT_X8_D24_UNORM_PACK32:
	case VK_FORMAT_D32_SFLOAT:
		return VK_IMAGE_ASPECT_DEPTH_BIT;
	case VK_FORMAT_S8_UINT:
		return VK_IMAGE_ASPECT_STENCIL_BIT;
	case VK_FORMAT_D16_UNORM_S8_UINT:
	case VK_FORMAT_D24_UNORM_S8_UINT:
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	default:
		return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}

VkImageLayout GetVkImageLayoutFrom(const ResourceState resourceState)
{
	if (resourceState == ResourceState::None)
	{
		return VK_IMAGE_LAYOUT_UNDEFINED;
	}
	else if (resourceState == ResourceState::TransferSrc)
	{
		return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	}
	else if (resourceState == ResourceState::TransferDst)
	{
		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	}
	else if (resourceState == ResourceState::DepthStencilRead)
	{
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	}
	else if (resourceState == ResourceState::DepthStencilWrite)
	{
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}
	else if (resourceState == ResourceState::GenericRead)
	{
		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
	else if (resourceState == ResourceState::RenderTarget)
	{
		return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	else if (resourceState == ResourceState::Present)
	{
		return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}
	else if (resourceState == ResourceState::GeneralComputeRead || 
			 resourceState == ResourceState::GeneralComputeWrite ||
			 resourceState == ResourceState::GeneralComputeReadWrite)
	{
		return VK_IMAGE_LAYOUT_GENERAL;
	}

	ASSERT(false, "Not supported ResourceState.");
	return VK_IMAGE_LAYOUT_UNDEFINED;
}

VkAccessFlags GetVkAccessFlagsFrom(const ResourceState resourceState)
{
	if (resourceState == ResourceState::None)
	{
		return VkAccessFlagBits(0);
	}
	else if (resourceState == ResourceState::BufferRead)
	{
		return VK_ACCESS_SHADER_READ_BIT;
	}
	else if (resourceState == ResourceState::BufferWrite)
	{
		return VK_ACCESS_SHADER_WRITE_BIT;
	}
	else if (resourceState == ResourceState::BufferReadWrite)
	{
		return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	}
	else if (resourceState == ResourceState::TransferSrc)
	{
		return VK_ACCESS_TRANSFER_READ_BIT;
	}
	else if (resourceState == ResourceState::TransferDst)
	{
		return VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (resourceState == ResourceState::DepthStencilRead)
	{
		return VkAccessFlagBits(VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT);
	}
	else if (resourceState == ResourceState::DepthStencilWrite)
	{
		return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else if (resourceState == ResourceState::GenericRead)
	{
		return VkAccessFlagBits(VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT |
			VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT |
			VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT);
	}
	else if (resourceState == ResourceState::RenderTarget)
	{
		return VkAccessFlagBits(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
	}
	else if (resourceState == ResourceState::Present)
	{
		return VK_ACCESS_MEMORY_READ_BIT;
	}

	ASSERT(false, "Not supported ResourceState.");
	return VK_IMAGE_LAYOUT_UNDEFINED;
}

VkPipelineStageFlagBits GetGraphicsPipelineStage(const ResourceState resourceState)
{
	switch (resourceState)
	{
	case ResourceState::TransferSrc:
	case ResourceState::TransferDst:
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	case ResourceState::DepthStencilRead:
		return VkPipelineStageFlagBits(
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
			VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
			VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
	case ResourceState::DepthStencilWrite:
		return VkPipelineStageFlagBits(
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
			VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
	case ResourceState::GenericRead:
		return VkPipelineStageFlagBits(
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
			VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
			VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
			VK_PIPELINE_STAGE_TRANSFER_BIT);
	case ResourceState::RenderTarget:
		return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	case ResourceState::None:
	case ResourceState::Present:
		return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	default:
		ASSERT(false, "Not supported ResourceState.");
	}

	return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
}

VkPipelineStageFlagBits GetTransferPipelineStage(const ResourceState resourceState)
{
	if ((resourceState == ResourceState::TransferSrc) || (resourceState == ResourceState::TransferDst))
	{
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	}

	return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
}


VkPrimitiveTopology GetVkPrimitiveTopologyFrom(const Topology topology)
{
	switch (topology)
	{
	case Topology::PointList:
		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case Topology::LineList:
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case Topology::LineStrip:
		return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	case Topology::TriangleList:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	case Topology::TriangleStrip:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	case Topology::PatchList:
		return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	default:
		ASSERT(false, "Not supported Topology.");
		return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
	}
}

VkSampleCountFlagBits GetVkSampleFlagsFrom(const MultisampleType multiSampleType)
{
	switch (multiSampleType)
	{
	case MultisampleType::Sample_1:
		return VK_SAMPLE_COUNT_1_BIT;
	case MultisampleType::Sample_2:
		return VK_SAMPLE_COUNT_2_BIT;
	case MultisampleType::Sample_4:
		return VK_SAMPLE_COUNT_4_BIT;
	case MultisampleType::Sample_8:
		return VK_SAMPLE_COUNT_8_BIT;
	default:
		ASSERT(false, "Not supported MultisampleType.");
		return VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
	}
}

VkCompareOp GetVkCompareOperationFrom(const CompareOperation compareOperation)
{
	switch (compareOperation)
	{
	case CompareOperation::Never:
		return VK_COMPARE_OP_NEVER;
	case CompareOperation::Less:
		return VK_COMPARE_OP_LESS;
	case CompareOperation::Equal:
		return VK_COMPARE_OP_EQUAL;
	case CompareOperation::LessEqual:
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	case CompareOperation::Greater:
		return VK_COMPARE_OP_GREATER;
	case CompareOperation::NotEqual:
		return VK_COMPARE_OP_NOT_EQUAL;
	case CompareOperation::GreaterEqual:
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case CompareOperation::Always:
		return VK_COMPARE_OP_ALWAYS;
	default:
		ASSERT(false, "Not supported CompareOperation.");
		return VK_COMPARE_OP_MAX_ENUM;
	}
}

VkStencilOp GetVkStencilOpFrom(const StencilOperation stencilOperation)
{
	switch (stencilOperation)
	{
	case StencilOperation::Keep:
		return VK_STENCIL_OP_KEEP;
	case StencilOperation::Zero:
		return VK_STENCIL_OP_ZERO;
	case StencilOperation::Replace:
		return VK_STENCIL_OP_REPLACE;
	case StencilOperation::IncrementAndClamp:
		return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
	case StencilOperation::DecrementAndClamp:
		return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
	case StencilOperation::Invert:
		return VK_STENCIL_OP_INVERT;
	case StencilOperation::Increment:
		return VK_STENCIL_OP_INCREMENT_AND_WRAP;
	case StencilOperation::Decrement:
		return VK_STENCIL_OP_DECREMENT_AND_WRAP;
	default:
		ASSERT(false, "Not supported StencilOperation.");
		return VK_STENCIL_OP_MAX_ENUM;
	}
}

VkFilter GetVkFilterFrom(const FilterType filterType)
{
	switch (filterType)
	{
	case FilterType::Point:
		return VK_FILTER_NEAREST;
	case FilterType::Linear:
		return VK_FILTER_LINEAR;
	default:
		ASSERT(false, "Not supported FilterType.");
		return VK_FILTER_MAX_ENUM;
	}
}

VkSamplerMipmapMode GetVkMipmapModeFrom(const FilterType filterType)
{
	switch (filterType)
	{
	case FilterType::Point:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	case FilterType::Linear:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	default:
		ASSERT(false, "Not supported FilterType.");
		return VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;
	}
}

VkSamplerAddressMode GetVkAddressModeFrom(const AddressMode addressMode)
{
	switch (addressMode)
	{
	case AddressMode::Repeat:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case AddressMode::Mirror:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case AddressMode::Clamp:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case AddressMode::Border:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	case AddressMode::MirrorClamp:
		return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	default:
		ASSERT(false, "Not supported AddressMode.");
		return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
	}
}

VkBorderColor GetVkBorderColorFrom(const Color color)
{
	if (color.value[0] == 0.0f && color.value[1] == 0.0f && color.value[2] == 0.0f)
	{
		return color.value[3] == 1.0f ? VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK : VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	}
	else if (color.value[0] == 1.0f && color.value[1] == 1.0f && color.value[2] == 1.0f && color.value[3] == 1.0f)
	{
		return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	}
	else
	{
		ASSERT(false, "Not supported border Color.");
		return VK_BORDER_COLOR_MAX_ENUM;
	}
}

bool IsDepthFormat(const Format format)
{
	return (format == Format::D32_SFLOAT || format == Format::D32_SFLOAT);
}

VkPipelineStageFlags GetVkPipelineStageFromResourceStageAndState(const ResourceStage stage, const ResourceState state, bool isSrcStage)
{
	if (state == ResourceState::DepthStencilWrite || state == ResourceState::DepthStencilRead)
		return isSrcStage ? VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT : VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

	switch (stage)
	{
	case ResourceStage::None:
		return VK_PIPELINE_STAGE_NONE; //TODO: double check this
	case ResourceStage::Compute:
		return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	case ResourceStage::Graphics:
		return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; //TODO: fix this, but this should be ok for now
	case ResourceStage::Transfer:
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	case ResourceStage::Undefined:
		return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	default:
		ASSERT(false, "Not supported resource stage");
		return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}
}

VkAccessFlags GetVkAccessFlagsFromResourceState(const ResourceState state)
{
	switch (state)
	{
	case ResourceState::None:
		return VK_ACCESS_NONE_KHR;
	case ResourceState::GenericRead:
		return VK_ACCESS_SHADER_READ_BIT;
	case ResourceState::RenderTarget:
		return VK_ACCESS_SHADER_WRITE_BIT;
	case ResourceState::GeneralComputeRead:
		return VK_ACCESS_SHADER_READ_BIT;
	case ResourceState::GeneralComputeWrite:
		return VK_ACCESS_SHADER_WRITE_BIT;
	case ResourceState::GeneralComputeReadWrite:
		return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	case ResourceState::TransferDst:
		return VK_ACCESS_TRANSFER_WRITE_BIT;
	case ResourceState::TransferSrc:
		return VK_ACCESS_TRANSFER_READ_BIT;
	case ResourceState::DepthStencilRead:
		return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
	case ResourceState::DepthStencilWrite:
		return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	default:
		ASSERT(false, "Not supported access state.");
		return VK_ACCESS_NONE_KHR;
	}
}

enum VkPipelineBindPoint GetVkBindPointFrom(const PipelineType pipelineType)
{
	switch (pipelineType)
	{
	case PipelineType::Graphics:
		return VK_PIPELINE_BIND_POINT_GRAPHICS;
	case PipelineType::Compute:
		return VK_PIPELINE_BIND_POINT_COMPUTE;
	default:
		ASSERT(false, "Unknown pipeline type.");
		return VK_PIPELINE_BIND_POINT_MAX_ENUM;
	}
}

VkBlendFactor GetVkBlendFactorFrom(const BlendValue blendValue)
{
	switch (blendValue)
	{
	case BlendValue::Zero:
		return VK_BLEND_FACTOR_ZERO;
	case BlendValue::One:
		return VK_BLEND_FACTOR_ONE;
	case BlendValue::SrcColor:
		return VK_BLEND_FACTOR_SRC_COLOR;
	case BlendValue::InvSrcColor:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	case BlendValue::SrcAlpha:
		return VK_BLEND_FACTOR_SRC_ALPHA;
	case BlendValue::InvSrcAlpha:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case BlendValue::DestAlpha:
		return VK_BLEND_FACTOR_DST_ALPHA;
	case BlendValue::InvDestAlpha:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	case BlendValue::DstColor:
		return VK_BLEND_FACTOR_DST_COLOR;
	case BlendValue::InvDstColor:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	case BlendValue::BlendFactor:
		return VK_BLEND_FACTOR_CONSTANT_COLOR;
	case BlendValue::InvBlendFactor:
		return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
	case BlendValue::SrcAlphaSat:
		return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
	case BlendValue::Src1Color:
		return VK_BLEND_FACTOR_SRC1_COLOR;
	case BlendValue::InvSrc1Color:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
	case BlendValue::Src1Alpha:
		return VK_BLEND_FACTOR_SRC1_ALPHA;
	case BlendValue::InvSrc1Alpha:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
	default:
		ASSERT(false, "Not supported BlendValue.");
		return VK_BLEND_FACTOR_MAX_ENUM;
	}
}

VkBlendOp GetVkBlendOpFrom(const BlendOperation blendOperation)
{
	switch (blendOperation)
	{
	case BlendOperation::Add:
		return VK_BLEND_OP_ADD;
	case BlendOperation::Subtract:
		return VK_BLEND_OP_SUBTRACT;
	case BlendOperation::ReverseSubtract:
		return VK_BLEND_OP_REVERSE_SUBTRACT;
	case BlendOperation::Min:
		return VK_BLEND_OP_MIN;
	case BlendOperation::Max:
		return VK_BLEND_OP_MAX;
	default:
		ASSERT(false, "Not supported BlendOperation.");
		return VK_BLEND_OP_MAX_ENUM;
	}
}

VkVertexInputRate GetVkVertexInputRateFrom(const VertexInputRate inputRate)
{
	switch (inputRate)
	{
	case VertexInputRate::PerVertex:
		return VK_VERTEX_INPUT_RATE_VERTEX;
	case VertexInputRate::PerInstance:
		return VK_VERTEX_INPUT_RATE_INSTANCE;
	default:
		ASSERT(false, "Not supported VertexInputRate.");
		return VK_VERTEX_INPUT_RATE_MAX_ENUM;
	}
}

VkClearValue GetVkClearColorValueFor(const Format format)
{
	switch (format)
	{
	case Format::R8G8B8A8_UNORM:
	case Format::R8G8B8A8_UNORM_SRGB:
	case Format::B8G8R8A8_UNORM:
	case Format::B8G8R8A8_UNORM_SRGB:
	case Format::R16G16B16A16_FLOAT:
	case Format::R16G16B16A16_UNORM:
	case Format::R32G32B32A32_FLOAT:
		return VkClearValue{ 0.5f, 0.5f, 0.5f, 1.0f };
	case Format::R8_UNORM:
	case Format::R32_SFLOAT:
		return VkClearValue{ 0.0f };
	case Format::R32G32B32_FLOAT:
	case Format::R11G11B10_FLOAT:
		return VkClearValue{ 0.1f, 0.1f, 0.1f };
	case Format::R32G32_FLOAT:
	case Format::R16G16_FLOAT:
		return VkClearValue{ 0.0f, 0.0f };
	case Format::R32_UINT:
		return VkClearValue{ 0 };
	case Format::D32_SFLOAT:
	case Format::D32_SFLOAT_S8_UINT:
	{
		VkClearValue cv{};
		cv.depthStencil.depth = 1.f;
		cv.depthStencil.stencil = 0;
		return cv;
	}
	default:
		ASSERT(false, "Not supported format.");
		return VkClearValue{ 0 };
	}
}

ClearValue GetClearColorValueFor(const Format format)
{
	VkClearValue vkcv = GetVkClearColorValueFor(format);
	ClearValue cv{};
	memcpy(&cv, &vkcv, sizeof(VkClearValue));
	return cv;
}

uint32_t GetBPPFrom(const Format format)
{
	switch (format)
	{
	case Format::R8G8B8A8_UNORM:
	case Format::R8G8B8A8_UNORM_SRGB:
	case Format::B8G8R8A8_UNORM:
	case Format::B8G8R8A8_UNORM_SRGB:
		return 4;
	case Format::R16G16B16A16_FLOAT:
		return 8;
	case Format::R32G32B32A32_FLOAT:
		return 16;
	default:
		ASSERT(false, "Cannot get BPP from format");
		return 4;
	}
}
