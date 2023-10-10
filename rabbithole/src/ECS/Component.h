#pragma once

#include "common.h"
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

    ComponentTypeID GetType() { return m_TypeId; }
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
    Vector3f rotationPosition;
    Vector3f position;

};

struct InputComponent : public Component
{
	std::vector<InputAction> inputActions;
	double mouse_x;
	double mouse_y;
    double mouse_current_x;
    double mouse_current_y;

};

struct CameraComponent : public Component {

    Matrix44f m_View;
    Matrix44f m_Projection;
    Vector3f m_Position;
};

struct MoverComponent : public Component {

    float m_MoveSpeed;
    float m_RotationSpeed;
};

struct MaterialComponent : public Component {

    Vector3f m_diffuse;
    Vector3f m_ambient;
    Vector3f m_specular;
};