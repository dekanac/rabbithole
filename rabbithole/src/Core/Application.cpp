#include "Application.h"
#include "Render/Renderer.h"
#include "Render/RenderSystem.h"
#include "Render/Window.h"
#include "ECS/EntityManager.h"
#include "Input/InputManager.h"
#include "Logger/Logger.h"

#include <GLFW/glfw3.h>
#include <SDL.h>

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
	
	SDL_Init(SDL_INIT_TIMER | SDL_INIT_EVENTS);
    
    glfwSetErrorCallback(ErrorCallback);
    const int ret = glfwInit();
    if (GL_FALSE == ret)
    {
        LOG_CRITICAL("GLFW Error!n");
    }

	
    if (!Window::instance().Init()) 
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
	SDL_Event event{};

	while (m_IsRunning)
	{
		if (glfwWindowShouldClose(Window::instance().GetNativeWindowHandle())) 
        {
			m_IsRunning = false;
		}

		SDL_PollEvent(&event);

		auto frameTime = glfwGetTimerValue();
		float deltaTime = (frameTime - previousFrameTime)  / static_cast<float>(glfwGetTimerFrequency());
		
#ifdef _DEBUG
		if (frameTime - previousOutputTime > 10000000) 
        {
			std::cout << "FPS:" << 1.f / deltaTime << std::endl;
			previousOutputTime = frameTime;

            PrintUsage();
		}
#endif
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
