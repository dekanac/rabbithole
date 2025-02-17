#include "InputManager.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Input/InputAction.h"
#include "Logger/Logger.h"
#include "Render/Window.h"

#include <GLFW/glfw3.h>
#include <iostream>

static float LAST_MOUSE_POS_Y = Window::instance().GetExtent().height / 2.f;
static float LAST_MOUSE_POS_X = Window::instance().GetExtent().width / 2.f;

bool KeyDown(KeyboardButton key)
{
    GLFWwindow* window = Window::instance().GetNativeWindowHandle();
    return glfwGetKey(window, static_cast<int>(key)) == GLFW_PRESS;
}

bool MouseButtonDown(int button)
{
    GLFWwindow* window = Window::instance().GetNativeWindowHandle();
    return glfwGetMouseButton(window, button) == GLFW_PRESS;
}

bool InputManager::Init()
{
    glfwSetInputMode(Window::instance().GetNativeWindowHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    InitKeybinds();

    return true;
}

void InputManager::ProcessMousePosition(double& x, double& y)
{
    glfwGetCursorPos(Window::instance().GetNativeWindowHandle(), &x, &y);
}

void InputManager::ProcessInput()
{
    for (auto& [action, key] : m_InputActions)
    {
        bool isPressed;
        if (key >= GLFW_MOUSE_BUTTON_1 && key <= GLFW_MOUSE_BUTTON_LAST)
        {
            isPressed = MouseButtonDown(key);
        }
        else
        {
            isPressed = KeyDown(key);
        }

        switch (m_InputActionStates[action])
        {
        case EInputActionState::None:
            m_InputActionStates[action] =
                isPressed ? EInputActionState::JustPressed : EInputActionState::None;
            break;
        case EInputActionState::JustPressed:
        case EInputActionState::Pressed:
            m_InputActionStates[action] =
                isPressed ? EInputActionState::Pressed : EInputActionState::Released;
            break;
        case EInputActionState::Released:
            m_InputActionStates[action] =
                isPressed ? EInputActionState::JustPressed : EInputActionState::None;
            break;
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

    double offset_x = curr_mouse_x - LAST_MOUSE_POS_X;
    double offset_y = curr_mouse_y - LAST_MOUSE_POS_Y;

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

bool InputManager::IsButtonActionActive(EInputAction action, EInputActionState state) const
{
    auto it = m_InputActionStates.find(action);
    ASSERT(it != m_InputActionStates.end(), "Unknown input action: {0}", action);
    return it != m_InputActionStates.end() && it->second == state;
}

void InputManager::InitKeybinds()
{
    m_InputActionStates.clear();
    m_InputActions.clear();

    m_InputActions["CameraUp"] = GLFW_KEY_W;
    m_InputActions["CameraLeft"] = GLFW_KEY_A;
    m_InputActions["CameraDown"] = GLFW_KEY_S;
    m_InputActions["CameraRight"] = GLFW_KEY_D;
    m_InputActions["Test"] = GLFW_KEY_X;
    m_InputActions["Debug1"] = GLFW_KEY_1;
    m_InputActions["Debug2"] = GLFW_KEY_2;
    m_InputActions["Debug3"] = GLFW_KEY_3;
    m_InputActions["Debug4"] = GLFW_KEY_4;
    m_InputActions["ActivateCameraMove"] = GLFW_KEY_LEFT_ALT;
    m_InputActions["ActivateCameraOrbit"] = GLFW_MOUSE_BUTTON_LEFT;
    m_InputActions["ActivateCameraZoom"] = GLFW_MOUSE_BUTTON_RIGHT;
    m_InputActions["ActivateCameraPan"] = GLFW_MOUSE_BUTTON_MIDDLE;
}

bool InputManager::IsActionActive(InputComponent* inputComponent, EInputAction targetAction)
{
    auto found = std::find_if(std::begin(inputComponent->inputActions),
                              std::end(inputComponent->inputActions), [targetAction](InputAction e)
                              { return e.m_Action == targetAction && e.m_Active; });

    return found != std::end(inputComponent->inputActions);
}