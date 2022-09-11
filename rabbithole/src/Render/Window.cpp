#include "Window.h"

#include "Logger/Logger.h"
#include "Render/Renderer.h"

bool Window::Init(const WindowData& windowData)
{
	LOG_INFO("Initializing Window");

	m_WindowData = windowData;
	ASSERT(m_WindowData.width > 0 && m_WindowData.height > 0, "Window size must be greater than zero");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_NativeWindowHandle = glfwCreateWindow(
		windowData.width, 
		windowData.height, 
		"Rabbithole", 
		nullptr, //repair for full screen
		nullptr
	);

	if (m_NativeWindowHandle == nullptr)
	{
		glfwTerminate();
		LOG_CRITICAL("Unable to create a window.");
		return false;
	}

    glfwSetWindowUserPointer(m_NativeWindowHandle, this);
    glfwSetFramebufferSizeCallback(m_NativeWindowHandle, FramebufferResizeCallback);

	return true;
}

void Window::FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
    
	Renderer::instance().m_FramebufferResized = true;

	Window::instance().m_WindowData.width = width;
	Window::instance().m_WindowData.height = height;
}

bool Window::Shutdown()
{
	LOG_INFO("Shutting down Window");

	if (m_NativeWindowHandle != nullptr)
	{
		glfwDestroyWindow(m_NativeWindowHandle);
	}

	m_NativeWindowHandle = nullptr;

	return true;
}
