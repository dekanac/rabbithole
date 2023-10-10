#pragma once

#include "common.h"

#include <memory>

class Application
{
private:
	bool m_IsRunning = false;
	
public:
	Application() {}
	~Application() {}
	bool Init();
	void Run();
	void Shutdown();
};

