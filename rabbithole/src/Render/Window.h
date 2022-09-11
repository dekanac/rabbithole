#pragma once
#include "common.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

constexpr uint32_t DEFAULT_WIDTH = 1280;
constexpr uint32_t DEFAULT_HEIGHT = 720;

struct WindowData
{
	std::string title{ "3D game engine" };
	uint32_t	width{ DEFAULT_WIDTH };
	uint32_t	height{ DEFAULT_HEIGHT };
	bool		vsync{ false };
};

class Window
{
	SingletonClass(Window);

public:
	bool Init(const WindowData& windowData = WindowData());
    bool Shutdown();
	
	GLFWwindow*		GetNativeWindowHandle() const { return m_NativeWindowHandle; }
	WindowExtent	GetExtent() { return WindowExtent{ m_WindowData.width, m_WindowData.height }; }
	bool			GetVSyncEnabled() { return m_WindowData.vsync; }
    
	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

private:
	WindowData	m_WindowData{};
	GLFWwindow* m_NativeWindowHandle{};
};

