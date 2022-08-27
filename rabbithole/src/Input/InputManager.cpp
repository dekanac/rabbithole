#include "InputManager.h"
#include "Logger/Logger.h"
#include "Input/InputAction.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Render/Window.h"

#include <SDL.h>

#include <iostream>

static float LAST_MOUSE_POS_Y = Window::instance().GetExtent().height / 2.f;
static float LAST_MOUSE_POS_X = Window::instance().GetExtent().width / 2.f;

bool KeyDown(KeyboardButton _iKey)
{   
    short iState = GetAsyncKeyState(static_cast<int>(_iKey));
    return (iState & 0x8000) != 0;
}

bool InputManager::Init()
{
    glfwSetInputMode(Window::instance().GetNativeWindowHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    InitKeybinds();

    return true;
}

void InputManager::ProcessMousePosition(double& x, double& y) {

    glfwGetCursorPos(Window::instance().GetNativeWindowHandle(), &x, &y);
}

void InputManager::ProcessInput()
{

    for (auto& [action, key] : m_InputActions)
    {
        bool bIsPressed = KeyDown(key);
        switch (m_InputActionStates[action])
        {
        case EInputActionState::None:
        {
            m_InputActionStates[action] = bIsPressed ? EInputActionState::JustPressed : EInputActionState::None;
            break;
        }
        case EInputActionState::JustPressed:
        case EInputActionState::Pressed:
        {
            m_InputActionStates[action] = bIsPressed ? EInputActionState::Pressed : EInputActionState::Released;
            break;
        }
        case EInputActionState::Released:
        {
            m_InputActionStates[action] = bIsPressed ? EInputActionState::JustPressed : EInputActionState::None;
            break;
        }
        default:
            ASSERT("Unknown EInputActionState {0}", m_InputActionStates[action]);
            m_InputActionStates[action] = EInputActionState::None;
            break;
        }
    }
}

void InputManager::Update(float dt)
{
    ProcessInput();
	
    double curr_mouse_x, curr_mouse_y;
    ProcessMousePosition(curr_mouse_x, curr_mouse_y);

    double offset_x, offset_y;
    offset_x = curr_mouse_x - LAST_MOUSE_POS_X;
    offset_y = curr_mouse_y - LAST_MOUSE_POS_Y;


    // Update entities
    auto inputComponents = EntityManager::instance().GetAllComponentInstances<InputComponent>();

    for (auto component : inputComponents)
    {
        component->mouse_x = offset_x;
        component->mouse_y = offset_y;
        component->mouse_current_x = curr_mouse_x;
        component->mouse_current_y = curr_mouse_y;

        for (auto& action : component->inputActions)
        {
            action.m_Active = IsButtonActionActive(action.m_Action, action.m_ActionTriggerState);
        }
    }

    LAST_MOUSE_POS_X = static_cast<float>(curr_mouse_x);
    LAST_MOUSE_POS_Y = static_cast<float>(curr_mouse_y);
  
}

bool InputManager::Shutdown()
{
    m_InputActions.clear();
    m_InputActionStates.clear();

    return true;
}

bool InputManager::IsButtonActionActive(EInputAction _eAction, EInputActionState _eState) const
{
    ASSERT(m_InputActionStates.find(_eAction) != m_InputActionStates.end(), "Unknown input action: {}", _eAction);
    return m_InputActionStates.at(_eAction) == _eState;
}

void InputManager::InitKeybinds()
{
    m_InputActionStates.clear();
    m_InputActions.clear();

    m_InputActions["CameraUp"] = 'W';
    m_InputActions["CameraLeft"] = 'A';
    m_InputActions["CameraDown"] = 'S';
    m_InputActions["CameraRight"] = 'D';
    m_InputActions["Test"] = 'X';
	m_InputActions["Debug1"] = '1';
	m_InputActions["Debug2"] = '2';
	m_InputActions["Debug3"] = '3';
	m_InputActions["Debug4"] = '4';
    m_InputActions["ActivateCameraMove"] = VK_LMENU;
    m_InputActions["ActivateCameraOrbit"] = VK_LBUTTON;
	m_InputActions["ActivateCameraZoom"] = VK_RBUTTON;
	m_InputActions["ActivateCameraPan"] = VK_MBUTTON;

}

bool InputManager::IsActionActive(InputComponent* inputComponent, EInputAction targetAction)
{
    auto found = std::find_if(
        std::begin(inputComponent->inputActions),
        std::end(inputComponent->inputActions),
        [targetAction](InputAction e)
    {
        return e.m_Action == targetAction && e.m_Active == true;
    });

    return found != std::end(inputComponent->inputActions);
}
