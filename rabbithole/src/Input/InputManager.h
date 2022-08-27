#pragma once
#include "common.h"

#include "Input/InputAction.h"

#include <unordered_map>

using KeyboardButton = int;

class Window;
class EntityManager;
struct InputComponent;

class InputManager
{
	SingletonClass(InputManager);
public:
	bool Init();
	void Update(float dt);
	bool Shutdown();

	static bool IsActionActive(InputComponent* inputComponent, EInputAction targetAction);
	bool IsButtonActionActive(EInputAction _eAction, EInputActionState _eState) const;
private:
	void ProcessInput();
	void ProcessMousePosition(double& x, double& y);
	void InitKeybinds();

	std::unordered_map<EInputAction, KeyboardButton> m_InputActions{ };
	std::unordered_map<EInputAction, EInputActionState> m_InputActionStates{ };
	
};
