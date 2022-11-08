#pragma once

#include <vector>
#include <unordered_map>

#include "Render/Renderer.h"

class Renderer;
class VulkanTexture;
class VulkanBuffer;

class RabbitPass
{
public:
	virtual void DeclareResources() = 0;
	virtual void Setup() = 0;
	virtual void Render() = 0;
	virtual const char* GetName() = 0;

protected:
	RabbitPass(Renderer& renderer) : m_Renderer(renderer) {}

	void SetCombinedImageSampler(uint32_t slot, VulkanTexture* texture, uint32_t mipSlice = 0);
	void SetSampledImage(uint32_t slot, VulkanTexture* texture, uint32_t mipSlice = 0);
	void SetStorageImage(uint32_t slot, VulkanTexture* texture, uint32_t mipSlice = 0);
	void SetConstantBuffer(uint32_t slot, VulkanBuffer* buffer);
	void SetStorageBufferRead(uint32_t slot, VulkanBuffer* buffer);
	void SetStorageBufferWrite(uint32_t slot, VulkanBuffer* buffer);
	void SetStorageBufferReadWrite(uint32_t slot, VulkanBuffer* buffer);
	void SetSampler(uint32_t slot, VulkanTexture* texture);
	void SetRenderTarget(uint32_t slot, VulkanTexture* texture);
	void SetDepthStencil(VulkanTexture* texture);

	Renderer& m_Renderer;
};

#define BEGIN_DECLARE_RABBITPASS(name) \
class name : public RabbitPass \
{ \
	NonCopyableAndMovable(name); \
public: \
	name(Renderer& renderer) : RabbitPass(renderer) {} \
	virtual void DeclareResources() override; \
	virtual void Setup() override; \
	virtual void Render() override; \
	virtual const char* GetName() override { return m_PassName; } \
private: \
	const char* m_PassName = #name; \
public:

#define END_DECLARE_RABBITPASS };

#define declareResource(name, type) static type* name
#define declareResourceArray(name, type, slices) static type* name[slices]
#define defineResource(pass, name, type) type* pass::name = nullptr;
#define defineResourceArray(pass, name, type, slices) type* pass::name[slices] = { nullptr };