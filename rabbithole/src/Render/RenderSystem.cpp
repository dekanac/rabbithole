#include "RenderSystem.h"

#include "Render/Renderer.h"
#include "Render/Window.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

bool RenderSystem::Init()
{
	if (Renderer::instance().Init())
	{
		LOG_INFO("Renderer successfully initialized.");
		return true;
	}
	else
	{
		LOG_CRITICAL("Renderer failed to initialize!");
		return false;
	}
}

void RenderSystem::Update(float dt)
{
    Renderer::instance().Draw(dt);
}

bool RenderSystem::Shutdown()
{
    if(Renderer::instance().Shutdown())
    {
        LOG_INFO("Renderer successfully shut down.");
    }
    else
    {
        LOG_CRITICAL("Renderer failed to shutdown!");
        return false;
    }

	return true;
}

