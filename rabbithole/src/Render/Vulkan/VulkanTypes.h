#pragma once

#include <memory>
#include "common.h"

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
	CPU,
	GPU,
	CPU2GPU,

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

enum class PipelineType : uint8_t
{
	Graphics,
	Compute,
	RayTracing,

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
	D32_SFLOAT,
	D32_SFLOAT_S8_UINT,
	B8G8R8A8_UNORM,
	B8G8R8A8_UNORM_SRGB,
	R8G8B8A8_UNORM,
	R8G8B8A8_UNORM_SRGB,
	R8_UNORM,
	R8_UINT,
	R32_SFLOAT,
	R16G16B16A16_FLOAT,
	R16G16B16A16_UNORM,
	R32G32B32A32_FLOAT,
	R32G32B32_FLOAT,
	R11G11B10_FLOAT,
	R32G32_FLOAT,
	R16G16_FLOAT,
	BC1_UNORM,
	BC1_UNORM_SRGB,
	BC2_UNORM,
	BC2_UNORM_SRGB,
	BC3_UNORM,
	BC3_UNORM_SRGB,
	BC5_UNORM,
	BC7_UNORM,
	BC7_UNORM_SRGB,
	R32_UINT,

	Count
};

enum class ColorSpace : uint8_t
{
	SRGB,
	ExtendedSRGB_Linear,
	ExtendedSRGB_Nonlinear,

	Count
};

enum class ResourceState : uint8_t
{
	None,
	TransferSrc,
	TransferDst,
	DepthStencilRead,
	DepthStencilWrite,
	GenericRead,
	RenderTarget,
	Present,
	GeneralComputeRead,
	GeneralComputeWrite,
	GeneralComputeReadWrite,
	BufferRead,
	BufferWrite,
	BufferReadWrite,

	Count
};

enum class ResourceStage : uint8_t
{
	None,
	Transfer,
	Graphics,
	Compute,
	RayTracing,
	Undefined,

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
	RayGen,
	Miss,
	ClosestHit,

	Count
};

enum class Topology : uint8_t
{
	PointList,
	LineList,
	LineStrip,
	TriangleList,
	TriangleStrip,
	PatchList,

	Count
};

enum class MultisampleType : uint8_t
{
	Sample_1,
	Sample_2,
	Sample_4,
	Sample_8,

	Count
};

enum class CompareOperation : uint8_t
{
	Never,
	Less,
	Equal,
	LessEqual,
	Greater,
	NotEqual,
	GreaterEqual,
	Always,

	Count
};

enum class StencilOperation : uint8_t
{
	Keep,
	Zero,
	Replace,
	IncrementAndClamp,
	DecrementAndClamp,
	Invert,
	Increment,
	Decrement,

	Count
};

enum class CullMode : uint8_t
{
	None,
	Back,
	Front,

	Count
};

enum class WindingOrder : uint8_t
{
	Clockwise,
	CounterClockwise,

	Count
};

enum class FilterType : uint8_t
{
	Point,
	Linear,

	Count
};

enum class AddressMode : uint8_t
{
	Repeat,
	Mirror,
	Clamp,
	Border,
	MirrorClamp,

	Count
};

enum class SamplerType : uint8_t
{
	Point,
	Bilinear,
	Trilinear,
	Anisotropic,
	
	Count
};

enum class BlendValue : uint8_t
{
	Zero,
	One,
	SrcColor,
	InvSrcColor,
	SrcAlpha,
	InvSrcAlpha,
	DestAlpha,
	InvDestAlpha,
	DstColor,
	InvDstColor,
	BlendFactor,
	InvBlendFactor,
	SrcAlphaSat,
	Src1Color,
	InvSrc1Color,
	Src1Alpha,
	InvSrc1Alpha,

	Count
};

enum class BlendOperation : uint8_t
{
	Add,
	Subtract,
	ReverseSubtract,
	Min,
	Max,

	Count
};

enum class VertexInputRate : uint8_t
{
	PerVertex,
	PerInstance,

	Count
};

enum class DescriptorType : uint8_t
{
	CombinedSampler = 0,
	UniformBuffer,
	StorageImage,
	StorageBuffer,
	SampledImage,
	Sampler,
	AccelerationStructure,

	Count
};

enum class BufferUsageFlags : uint16_t
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
	ShaderDeviceAddress = 0x1 << 8,
	AccelerationStructureStorage = 0x1 << 9,
	AccelerationStructureBuild = 0x1 << 10,
	ShaderBindingTable = 0x1 << 11
};
RABBITHOLE_FLAG_TYPE_SETUP(BufferUsageFlags);

enum class ImageFlags : uint8_t
{
	None = 0x0 << 0,
	CubeMap = 0x1 << 0,
	LinearTiling = 0x1 << 1,
};
RABBITHOLE_FLAG_TYPE_SETUP(ImageFlags)

enum class ImageUsageFlags : uint8_t
{
	None = 0x0 << 0,
	TransferSrc = 0x1 << 0,
	TransferDst = 0x1 << 1,
	Resource = 0x1 << 2,
	Storage = 0x1 << 3,
	RenderTarget = 0x1 << 4,
	DepthStencil = 0x1 << 5,
};
RABBITHOLE_FLAG_TYPE_SETUP(ImageUsageFlags)

enum class ImageViewFlags : uint8_t
{
	None = 0x0 << 0,
	Color = 0x1 << 0,
	ReadDepth = 0x1 << 1,
	ReadStencil = 0x1 << 2,
};
RABBITHOLE_FLAG_TYPE_SETUP(ImageViewFlags)

enum class ClearDepthStencilFlags : uint8_t
{
	None = 0x0 << 0,
	ClearDepth = 0x1 << 0,
	ClearStencil = 0x1 << 1,
	ClearDepthStencil = ClearDepth | ClearStencil,
};
RABBITHOLE_FLAG_TYPE_SETUP(ClearDepthStencilFlags)

enum class ColorWriteMaskFlags : uint8_t
{
	None = 0x0 << 0,
	R = 0x1 << 0,
	G = 0x1 << 1,
	B = 0x1 << 2,
	A = 0x1 << 3,
	RG = R | G,
	RGB = RG | B,
	RGBA = RGB | A,
};
RABBITHOLE_FLAG_TYPE_SETUP(ColorWriteMaskFlags)

enum class TextureFlags : uint16_t
{
	None = 0x0 << 0,
	Color = 0x1 << 0,
	DepthStencil = 0x1 << 1,
	CubeMap = 0x1 << 2,
	Read = 0x1 << 3,
	RenderTarget = 0x1 << 4,
	LinearTiling = 0x1 << 5,
	TransferSrc = 0x1 << 6,
	TransferDst = 0x1 << 7,
	Storage = 0x1 << 8
};
RABBITHOLE_FLAG_TYPE_SETUP(TextureFlags)


struct Color
{
	Vector4f value;
};

struct Extent3D
{
	uint32_t Width;
	uint32_t Height;
	uint32_t Depth;
};

struct Offset3D
{
	int32_t X;
	int32_t Y;
	int32_t Z;
};

struct ImageSubresourceRange
{
	uint32_t MipSlice;
	uint32_t MipSize;
	uint32_t ArraySlice;
	uint32_t ArraySize;
};

struct DepthStencil
{
	float Depth;
	uint8_t Stencil;
};

union ClearValue
{
	Color Color;
	DepthStencil DepthStencil;
};

enum class LoadOp : uint8_t
{
	DontCare = 0,
	Load = 1,
	Clear = 2
};
