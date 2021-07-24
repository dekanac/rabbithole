
#include "Window.h"
#include "Logger/Logger.h"
#include "Render/Renderer.h"

bool Window::Init(const WindowData& windowData_)
{
	LOG_INFO("Initializing Window");


	m_WindowData = windowData_;
	ASSERT(m_WindowData.m_Width > 0 && m_WindowData.m_Height > 0, "Window size must be greater than zero");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	
	m_VulkanExtent.width = m_WindowData.m_Width;
	m_VulkanExtent.height = m_WindowData.m_Height;

	m_NativeWindowHandle = glfwCreateWindow(
		windowData_.m_Width, 
		windowData_.m_Height, 
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
    
    auto app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    app->m_FramebufferResized = true;

	Window::instance().m_WindowData.m_Height = height;
	Window::instance().m_WindowData.m_Width = width;
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
