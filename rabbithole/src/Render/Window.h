#pragma once
#include "common.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

constexpr int DEFAULT_WIDTH = 1280;
constexpr int DEFAULT_HEIGHT = 720;

struct WindowData
{
	std::string m_Title{ "3D game engine" };
	int m_Width{ DEFAULT_WIDTH };
	int m_Height{ DEFAULT_HEIGHT };
	bool m_Vsync{ false };
};

class Window
{
    SingletonClass(Window)

public:
	bool Init(const WindowData& windowData_ = WindowData());
    static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
    bool Shutdown();
	GLFWwindow* GetNativeWindowHandle() const { return m_NativeWindowHandle; }
	WindowExtent GetExtent() { return m_Extent; }
	bool GetVSyncEnabled() { return m_WindowData.m_Vsync; }

private:
	WindowExtent m_Extent;
	WindowData m_WindowData{};
	GLFWwindow* m_NativeWindowHandle{ };

};

