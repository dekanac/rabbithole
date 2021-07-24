#pragma once
#include "common.h"

#include <memory>

class EntityManager;
class Window;

class RenderSystem
{	
    SingletonClass(RenderSystem)

public:
	bool Init();
	void Update(float dt);
	bool Shutdown();
};

