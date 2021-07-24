#pragma once

#define RABBITHOLE_FLAG_TYPE_SETUP(flagType) \
    inline flagType operator&(flagType a, flagType b) \
    { \
        return static_cast<flagType>(static_cast<std::underlying_type<flagType>::type>(a) & \
			static_cast<std::underlying_type<flagType>::type>(b) ); \
    } \
	\
    inline flagType operator|(flagType a, flagType b) \
    { \
        return static_cast<flagType>( static_cast<std::underlying_type<flagType>::type>(a) | \
			static_cast<std::underlying_type<flagType>::type>(b)); \
    } \
	\
    inline bool IsFlagSet(flagType x) \
    { \
        return (static_cast<uint32_t>(x) != 0); \
    }

enum class MemoryAccess : uint8_t
{
	Host,
	Device,

	Count
};

enum class QueueType : uint8_t
{
	Graphics,
	Present,
	Compute,
	Transfer,

	Count
};

enum class PresentResult : uint8_t
{
	Valid,
	Invalid,

	Count
};

enum class Format : uint8_t
{
	UNDEFINED,
	D32_SFLOAT_S8_UINT,
	B8G8R8A8_UNORM,
	B8G8R8A8_UNORM_SRGB,
	R8G8B8A8_UNORM,
	R8G8B8A8_UNORM_SRGB,
	R8_UNORM,
	R16G16B16A16_FLOAT,
	R32G32B32A32_FLOAT,
	R32G32B32_FLOAT,
	R32G32_FLOAT,
	BC1_UNORM,
	BC1_UNORM_SRGB,
	BC2_UNORM,
	BC2_UNORM_SRGB,
	BC3_UNORM,
	BC3_UNORM_SRGB,
	BC7_UNORM,
	BC7_UNORM_SRGB,

	Count
};

enum class ColorSpace : uint8_t
{
	SRGB,
	ExtendedSRGB_Linear,
	ExtendedSRGB_Nonlinear,

	Count
};

enum class DescriptorType : uint8_t
{
	CombinedSampler,
	UniformBuffer,

	Count
};

enum class ShaderType : uint8_t
{
	Vertex,
	Fragment,
	Geometry,
	Hull,
	Domain,
	Compute,

	Count
};


enum class BufferUsageFlags : uint8_t
{
	None = 0x0 << 0,
	TransferSrc = 0x1 << 0,
	TransferDst = 0x1 << 1,
	ResrcBuffer = 0x1 << 2,
	StorageBuffer = 0x1 << 3,
	VertexBuffer = 0x1 << 4,
	IndexBuffer = 0x1 << 5,
	UniformBuffer = 0x1 << 6,
	IndirectBuffer = 0x1 << 7,
};
RABBITHOLE_FLAG_TYPE_SETUP(BufferUsageFlags);

