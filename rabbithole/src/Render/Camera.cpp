#include "Camera.h"
#include "Render/Window.h"
#include "ECS/EntityManager.h"
#include "Input/InputManager.h"
#include "Logger/Logger.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <iostream>

static const float MAX_VERTICAL_ANGLE = 85.0f;

Camera::Camera() 
	: m_Position(0.0f, 0.0f, 0.0f)
	, m_FieldOfView(45.0f)
	, m_NearPlane(0.01f)
	, m_FarPlane(100.0f)
	, m_Aspect(static_cast<float>(Window::instance().GetExtent().width) / Window::instance().GetExtent().height)
	, m_Pitch(0.f)
	, m_Yaw(0.f)
	, m_ViewMatrix(rabbitMat4f{ 1.f })
	, m_FocalPoint({ 0.0f, 0.0f, 0.0f })
	, m_Distance(20.f)
{
	UpdateView();
}

bool Camera::Init() 
{
	Entity* mainCamera = new Entity();

	mainCamera->AddComponent<CameraComponent>();
	mainCamera->AddComponent<MoverComponent>();
	mainCamera->AddComponent<InputComponent>();

	auto moveComp = mainCamera->GetComponent<MoverComponent>();
	moveComp->m_MoveSpeed = 10.f;
	moveComp->m_RotationSpeed = 0.1f;

	auto inputComp = mainCamera->GetComponent<InputComponent>();
	inputComp->inputActions.push_back({ "CameraUp" });
	inputComp->inputActions.push_back({ "CameraDown" });
	inputComp->inputActions.push_back({ "CameraRight" });
	inputComp->inputActions.push_back({ "CameraLeft" });
	inputComp->inputActions.push_back({ "ActivateCameraMove", EInputActionState::Pressed });
	inputComp->inputActions.push_back({ "ActivateCameraOrbit", EInputActionState::Pressed });
	inputComp->inputActions.push_back({ "ActivateCameraZoom", EInputActionState::Pressed });
	inputComp->inputActions.push_back({ "ActivateCameraPan", EInputActionState::Pressed });

	auto camComp = mainCamera->GetComponent<CameraComponent>();
	
	EntityManager::instance().AddEntity(mainCamera);

	return true;
}

void Camera::Update(float dt)
{
	auto cameraEnt = EntityManager::instance().GetAllEntitiesWithComponent<CameraComponent>();
	auto cameraComp = cameraEnt[0]->GetComponent<CameraComponent>();
	
	//CAMERA MOVEMENT
	auto moveComp = cameraEnt[0]->GetComponent<MoverComponent>();
	auto cameraInput = cameraEnt[0]->GetComponent<InputComponent>();
	auto inputComp = cameraEnt[0]->GetComponent<InputComponent>();

	float cameraSpeed = moveComp->m_MoveSpeed * dt;

	if (InputManager::IsActionActive(cameraInput, "ActivateCameraMove"))
	{
		bool moveUpInput = InputManager::IsActionActive(cameraInput, "CameraUp");
		bool moveDownInput = InputManager::IsActionActive(cameraInput, "CameraDown");
		bool moveRightInput = InputManager::IsActionActive(cameraInput, "CameraRight");
		bool moveLeftInput = InputManager::IsActionActive(cameraInput, "CameraLeft");

		glm::vec2 delta = { inputComp->mouse_x * cameraSpeed, inputComp->mouse_y * cameraSpeed };

		if (InputManager::IsActionActive(cameraInput, "ActivateCameraPan"))
			MousePan(delta/3.f);
		else if (InputManager::IsActionActive(cameraInput, "ActivateCameraOrbit"))
			MouseRotate(delta);
		else if (InputManager::IsActionActive(cameraInput, "ActivateCameraZoom"))
			MouseZoom(delta.y);

		auto front = GetForwardDirection();
		auto up = GetUpDirection();

		if (moveUpInput) { m_FocalPoint += cameraSpeed * front; }
		if (moveDownInput) { m_FocalPoint -= cameraSpeed * front; }
		if (moveLeftInput) { m_FocalPoint -= glm::normalize(glm::cross(front, up)) * cameraSpeed; }
		if (moveRightInput) { m_FocalPoint += glm::normalize(glm::cross(front, up)) * cameraSpeed; }

		UpdateView();
	}
}

rabbitMat4f Camera::GetMatrix() const 
{
	return Projection() * View();
}

rabbitMat4f Camera::Projection() const 
{
	return glm::perspective(glm::radians(m_FieldOfView), m_Aspect, m_NearPlane, m_FarPlane);
}

rabbitMat4f Camera::View() const
{
	return m_ViewMatrix;
}

const rabbitVec3f& Camera::GetPosition() const 
{
	return CalculatePosition();
}

float Camera::GetFieldOfView() const 
{
	return m_FieldOfView;
}

void Camera::setFieldOfView(float fieldOfView_) 
{
	ASSERT(fieldOfView_ > 0.0f && fieldOfView_ < 180.0f, "Field of view mus be between 0 adn 180 degrees!");
	m_FieldOfView = fieldOfView_;
}

float Camera::GetNearPlane() const
{
	return m_NearPlane;
}

float Camera::GetfarPlane() const
{
	return m_FarPlane;
}

void Camera::setNearAndFarPlanes(float nearPlane_, float farPlane_)
{
	ASSERT(nearPlane_ > 0.0f, "Near plane must be greater than 0!");
	ASSERT(farPlane_ > nearPlane_, "Far plane must be greater then near plane!");
	m_NearPlane = nearPlane_;
	m_FarPlane = farPlane_;
}

void Camera::setViewportAspectRatio(float vpaspect_)
{
	ASSERT(vpaspect_ > 0.0, "");
	m_Aspect = vpaspect_;
}

glm::quat Camera::GetOrientation() const
{
	return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
}

glm::vec3 Camera::GetUpDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 Camera::CalculatePosition() const
{
	return m_FocalPoint - GetForwardDirection() * m_Distance;
}

glm::vec3 Camera::GetRightDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 Camera::GetForwardDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
}

void Camera::UpdateView()
{
	m_Position = CalculatePosition();
	glm::quat orientation = GetOrientation();
	m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
	m_ViewMatrix = glm::inverse(m_ViewMatrix);
}

void Camera::MousePan(const glm::vec2& delta)
{
	m_FocalPoint += -GetRightDirection() * delta.x * m_Distance;
	m_FocalPoint += GetUpDirection() * delta.y * m_Distance;
}

void Camera::MouseRotate(const glm::vec2& delta)
{
	float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
	m_Yaw += yawSign * delta.x;
	m_Pitch += delta.y;
}

void Camera::MouseZoom(float delta)
{
	m_Distance -= delta;
	if (m_Distance < 1.0f)
	{
		m_FocalPoint += GetForwardDirection();
		m_Distance = 1.0f;
	}
}
