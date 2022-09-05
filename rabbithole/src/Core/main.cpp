#define VMA_IMPLEMENTATION

#include "common.h"

#include "Core/Application.h"
#include "Render/Renderer.h"

int main() 
{
	auto app = std::make_unique<Application>();
	app->Init();
	app->Run();
	app->Shutdown();
}