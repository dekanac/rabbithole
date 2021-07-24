#pragma once
#include "Input/InputAction.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#include <type_traits>
#include <vector>

using ComponentTypeID = size_t;

class Entity;

struct Component
{
    Entity* m_Owner{};
    ComponentTypeID m_TypeId{ 0 };
    bool m_Active{ false };
    virtual ~Component() = default;

private:
    inline static ComponentTypeID m_MaxComponentTypeId = 0;

public:

    template <typename T>
    static ComponentTypeID GetComponentTypeID()
    {
        static_assert(std::is_base_of<Component, T>::value, "");
        static ComponentTypeID typeID = Component::m_MaxComponentTypeId++;
        return typeID;
    }
};



struct TransformComponent : public Component {

    float rotationAngle;
    glm::vec3 rotationPosition;
    glm::vec3 position;

};

struct InputComponent : public Component
{
	std::vector<InputAction> inputActions;
	double mouse_x;
	double mouse_y;

};

struct CameraComponent : public Component {

    glm::mat4 m_View;
    glm::mat4 m_Projection;
    glm::vec3 m_Position;
};

struct MoverComponent : public Component {

    float m_MoveSpeed;
    float m_RotationSpeed;
};

struct MaterialComponent : public Component {

    glm::vec3 m_diffuse;
    glm::vec3 m_ambient;
    glm::vec3 m_specular;
};