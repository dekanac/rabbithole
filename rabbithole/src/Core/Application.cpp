#include "Application.h"
#include "ECS/EntityManager.h"
#include "Input/InputManager.h"
#include "Logger/Logger.h"
#include "Render/RenderSystem.h"
#include "Render/Renderer.h"
#include "Render/Window.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>

void ErrorCallback(int, const char* err_str)
{
    LOG_CRITICAL("GLFW error: {}", err_str);
}

bool Application::Init()
{
	Logger::Init();
	LOG_INFO("Logger succesfully created.");
    
    glfwSetErrorCallback(ErrorCallback);
    const int ret = glfwInit();
    if (GL_FALSE == ret)
    {
        LOG_CRITICAL("GLFW Error!n");
    }

    WindowData wd{ 
        .title = "Rabbithole3D",
        .width = 1600, 
        .height = 900, 
        .vsync = false };

    if (!Window::instance().Init(wd))
    {
        LOG_CRITICAL("Window failed to initialize!");
    }

    else
    {
        LOG_INFO("Window successfully created");
    }
	
	if (!EntityManager::instance().Init())
    {
        LOG_CRITICAL("EntityManager failed to initialize!");
    }
    else
    {
        LOG_INFO("EntityManager initialized.");
    }

	if (!InputManager::instance().Init()) 
    {
		LOG_CRITICAL("InputManager failed to initialize!");
	}
	else 
    {
		LOG_INFO("InputManager initialized.");
	}

	if (!RenderSystem::instance().Init())
    { 
		
		LOG_CRITICAL("RenderSystem failed to initialize!"); 
	}
	else 
    { 
		LOG_INFO("RenderSystem initialized."); 
	}
	
    m_IsRunning = true;

	return true;
}

void Application::Run()
{
	auto previousFrameTime = glfwGetTimerValue();
	auto previousOutputTime = glfwGetTimerValue();

	while (m_IsRunning)
	{
		if (glfwWindowShouldClose(Window::instance().GetNativeWindowHandle())) 
        {
			m_IsRunning = false;
		}

		auto frameTime = glfwGetTimerValue();
		float deltaTime = (frameTime - previousFrameTime)  / static_cast<float>(glfwGetTimerFrequency());
		
#ifdef RABBITHOLE_DEBUG
		if (frameTime - previousOutputTime > 10000000) 
        {
			std::cout << "FPS:" << 1.f / deltaTime << std::endl;
			previousOutputTime = frameTime;
		}
#endif // RABBITHOLE_DEBUG
		//UPDATE GAME LOOP
		InputManager::instance().Update(deltaTime);
		RenderSystem::instance().Update(deltaTime);

		glfwPollEvents();
		
		previousFrameTime = frameTime;

	}

}

void Application::Shutdown()
{
    if (!InputManager::instance().Shutdown())
    {
        LOG_CRITICAL("InputManager failed to shutdown!");
    }
    else
    {
        LOG_INFO("InputManager successfully shutdown!");
    }
    if (!RenderSystem::instance().Shutdown())
    {
        LOG_CRITICAL("RenderSystem failed to shutdown!");
    }
    else
    {
        LOG_INFO("RenderSystem successfully shutdown!");
    }
    if (!Window      ::instance().Shutdown())
    {
        LOG_CRITICAL("Window failed to shutdown!");
    }
    else
    {
        LOG_INFO("Window successfully shutdown!");
    }

        
	glfwTerminate();
}
