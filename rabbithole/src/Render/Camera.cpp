#include "Camera.h"
#include "ECS/EntityManager.h"
#include "Input/InputManager.h"
#include "Logger/Logger.h"
#include "Render/Window.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>

#include <cmath>
#include <iostream>

static const float MAX_VERTICAL_ANGLE = 85.0f;

Camera::Camera()
	: m_Position(0.0f, 0.0f, 0.0f)
	, m_FieldOfView(45.0f)
	, m_NearPlane(0.01f)
	, m_FarPlane(1000.0f)
	, m_Aspect(static_cast<float>(Window::instance().GetExtent().width) / Window::instance().GetExtent().height)
	, m_Pitch(0.f)
	, m_Yaw(0.f)
	, m_ProjMatrix(rabbitMat4f{ 1.f })
	, m_ViewMatrix(rabbitMat4f{ 1.f })
	, m_FocalPoint({ 0.0f, 0.0f, 0.0f })
	, m_CameraPanSpeed(0.5f)
	, m_Distance(0.f)
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
	moveComp->m_MoveSpeed = 4.f; //camera sensitivity 

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
	m_ProjMatrix = glm::perspective(glm::radians(m_FieldOfView), m_Aspect, m_NearPlane, m_FarPlane);
	m_ProjMatrix[1][1] *= -1;
	
	auto cameraEnt = EntityManager::instance().GetAllEntitiesWithComponent<CameraComponent>();
	auto cameraComp = cameraEnt[0]->GetComponent<CameraComponent>();
	
	//CAMERA MOVEMENT
	auto moveComp = cameraEnt[0]->GetComponent<MoverComponent>();
	auto cameraInput = cameraEnt[0]->GetComponent<InputComponent>();
	auto inputComp = cameraEnt[0]->GetComponent<InputComponent>();

	float cameraSpeed = moveComp->m_MoveSpeed * 0.003f;

	if (InputManager::IsActionActive(cameraInput, "ActivateCameraMove"))
	{
		bool moveUpInput = InputManager::IsActionActive(cameraInput, "CameraUp");
		bool moveDownInput = InputManager::IsActionActive(cameraInput, "CameraDown");
		bool moveRightInput = InputManager::IsActionActive(cameraInput, "CameraRight");
		bool moveLeftInput = InputManager::IsActionActive(cameraInput, "CameraLeft");

		glm::vec2 delta = { inputComp->mouse_x * cameraSpeed, inputComp->mouse_y * cameraSpeed };

		if (InputManager::IsActionActive(cameraInput, "ActivateCameraPan"))
			MousePan(delta);
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
	return m_ProjMatrix * m_ViewMatrix;
}

rabbitMat4f Camera::Projection() const
{
	return m_ProjMatrix;
}

rabbitMat4f Camera::ProjectionJittered() const
{
	return m_ProjMatrixJittered;
}

rabbitMat4f Camera::View() const
{
	return m_ViewMatrix;
}

const rabbitVec3f Camera::GetPosition() const 
{
	return CalculatePosition();
}

float Camera::GetFieldOfView() const 
{
	return m_FieldOfView;
}

float Camera::GetFieldOfViewVerticalRad() const
{
	return (glm::radians(m_FieldOfView) / m_Aspect);
}

void Camera::SetFieldOfView(float fieldOfView) 
{
	ASSERT(fieldOfView > 0.0f && fieldOfView < 180.0f, "Field of view mus be between 0 adn 180 degrees!");
	m_FieldOfView = fieldOfView;
}

float Camera::GetNearPlane() const
{
	return m_NearPlane;
}

float Camera::GetFarPlane() const
{
	return m_FarPlane;
}

void Camera::SetNearAndFarPlanes(float nearPlane, float farPlane)
{
	ASSERT(nearPlane > 0.0f, "Near plane must be greater than 0!");
	ASSERT(farPlane > nearPlane, "Far plane must be greater then near plane!");
	m_NearPlane = nearPlane;
	m_FarPlane = farPlane;
}

void Camera::SetViewportAspectRatio(float vpaspect)
{
	ASSERT(vpaspect > 0.0, "");
	m_Aspect = vpaspect;
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
	m_FocalPoint += -GetRightDirection() * delta.x * m_Distance * m_CameraPanSpeed;
	m_FocalPoint += GetUpDirection() * delta.y * m_Distance * m_CameraPanSpeed;
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

void Camera::SetProjectionJitter(float jitterX, float jitterY)
{
	m_ProjMatrixJittered = m_ProjMatrix;
	m_ProjMatrixJittered[2][0] = jitterX;
	m_ProjMatrixJittered[2][1] = jitterY;
}