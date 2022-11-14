#pragma once

class Renderer;
class VulkanTexture;

#include <vulkan/vulkan.h>

#include <unordered_map>

class ImGuiManager
{
public:
	~ImGuiManager();

	void Init(Renderer& renderer);
	void Render(Renderer& renderer);
	void Shutdown();

	void NewFrame(float posX, float posY, float width, float height);
	void RegisterTextureForImGui(VulkanTexture* texture);
	VkDescriptorSet GetImGuiTextureFrom(VulkanTexture* texture);
	inline bool IsInitialized() const { return m_IsImguiInitialized; }
	inline bool IsReady() const { return m_ImGuiReady; }
	void MakeReady() { m_ImGuiReady = true; }

private:
	bool m_IsImguiInitialized = false;
	bool m_ImGuiReady = false;

	std::unordered_map<VulkanTexture*, VkDescriptorSet> m_RegisteredImGuiTextures;
};