#include "Core/Application.h"

#include <memory>

int main() 
{
	auto app = std::make_unique<Application>();
	app->Init();
	app->Run();
	app->Shutdown();
}
