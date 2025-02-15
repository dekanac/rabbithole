#include "Application.h"

#include "ECS/EntityManager.h"
#include "Input/InputManager.h"
#include "Logger/Logger.h"
#include "Render/RenderSystem.h"
#include "Render/Renderer.h"
#include "Render/Window.h"

#include <GLFW/glfw3.h>

#include <optick/src/optick.config.h>
#include <optick/src/optick.h>

#include <iostream>

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
        .width = 1920, 
        .height = 1080, 
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
    auto previousFrameTime = std::chrono::high_resolution_clock::now();
    auto previousOutputTime = previousFrameTime;

    while (m_IsRunning)
    {
        OPTICK_FRAME("MainThread");

        if (glfwWindowShouldClose(Window::instance().GetNativeWindowHandle()))
        {
            m_IsRunning = false;
            break;
        }

        auto currentFrameTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> deltaTime = currentFrameTime - previousFrameTime;
        previousFrameTime = currentFrameTime;

#ifdef RABBITHOLE_DEBUG
        // Output FPS every 2 seconds
        std::chrono::duration<float> elapsed = currentFrameTime - previousOutputTime;
        if (elapsed.count() >= 2.0f)
        {
            float fps = deltaTime.count() > 0.0f ? (1.0f / deltaTime.count()) : 0.0f;
            std::cout << std::fixed << std::setprecision(2) << "FPS: " << fps << std::endl;
            previousOutputTime = currentFrameTime;
        }
#endif // RABBITHOLE_DEBUG

        // UPDATE GAME LOOP
        InputManager::instance().Update(deltaTime.count());
        RenderSystem::instance().Update(deltaTime.count());

        glfwPollEvents();
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
    if (!Window::instance().Shutdown())
    {
        LOG_CRITICAL("Window failed to shutdown!");
    }
    else
    {
        LOG_INFO("Window successfully shutdown!");
    }

        
	glfwTerminate();
}
