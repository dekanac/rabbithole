#define VMA_IMPLEMENTATION

#include "common.h"

#include "Render/Vulkan/precomp.h"

#include "Core/Application.h"

#include "ECS/Component.h"
#include "ECS/Entity.h"
#include "ECS/EntityManager.h"

#include "Input/InputAction.h"
#include "Input/InputManager.h"

#include "Logger/Logger.h"

#include "Render/Camera.h"
#include "Render/Renderer.h"
#include "Render/RenderSystem.h"
#include "Render/Window.h"

int main() {
	
	auto app = std::make_unique<Application>();
	app->Init();
	app->Run();
	app->Shutdown();
}