#include "Camera.h"
#include "Render/Window.h"
#include "ECS/EntityManager.h"
#include "Input/InputManager.h"
#include "Logger/Logger.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <iostream>

static const float MAX_VERTICAL_ANGLE = 85.0f;

Camera::Camera() :
	m_Position(-2.0f, -2.0f, 2.0f),
	m_Front(0.f, 0.f, 0.f),
	m_UpVector(0.f, 1.f, 0.f),
	m_HorizontalAngle(0.0f),
	m_VerticalAngle(0.0f),
	m_FieldOfView(45.0f),
	m_NearPlane(0.01f),
	m_FarPlane(100.0f),
	m_Aspect(static_cast<float>(DEFAULT_WIDTH) / DEFAULT_HEIGHT)
{
}

const rabbitVec3f& Camera::getFront() const
{
	return m_Front;
}

const rabbitVec3f& Camera::getUpVector() const
{
	return m_UpVector;
}

void Camera::setFront(rabbitVec3f front_)
{
	m_Front = front_;
}

void Camera::setUpVector(rabbitVec3f upvec_)
{
	m_UpVector = upvec_;
}

bool Camera::Init() 
{

	Entity* mainCamera = new Entity();

	mainCamera->AddComponent<CameraComponent>();
	mainCamera->AddComponent<MoverComponent>();
	mainCamera->AddComponent<InputComponent>();

	auto moveComp = mainCamera->GetComponent<MoverComponent>();
	moveComp->m_MoveSpeed = 5.f;
	moveComp->m_RotationSpeed = 0.1f;

	auto inputComp = mainCamera->GetComponent<InputComponent>();
	inputComp->inputActions.push_back({ "CameraUp" });
	inputComp->inputActions.push_back({ "CameraDown" });
	inputComp->inputActions.push_back({ "CameraRight" });
	inputComp->inputActions.push_back({ "CameraLeft" });

	auto camComp = mainCamera->GetComponent<CameraComponent>();
	camComp->m_View = View();
	camComp->m_Projection = Projection();
	
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
	
	bool moveUpInput = InputManager::IsActionActive(cameraInput, "CameraUp");
	bool moveDownInput = InputManager::IsActionActive(cameraInput, "CameraDown");
	bool moveRightInput = InputManager::IsActionActive(cameraInput, "CameraRight");
	bool moveLeftInput = InputManager::IsActionActive(cameraInput, "CameraLeft");
	
	float cameraSpeed = moveComp->m_MoveSpeed * dt;
	if (moveUpInput) { m_Position += cameraSpeed * m_Front; }
	if (moveDownInput) { m_Position -= cameraSpeed * m_Front; }
	if (moveLeftInput) { m_Position -= glm::normalize(glm::cross(m_Front, m_UpVector)) * cameraSpeed; }
	if (moveRightInput) { m_Position += glm::normalize(glm::cross(m_Front, m_UpVector)) * cameraSpeed; }

	rabbitVec3f direction;
	direction.x = cos(glm::radians(m_HorizontalAngle)) * cos(glm::radians(m_VerticalAngle));
	direction.y = sin(glm::radians(m_VerticalAngle));
	direction.z = sin(glm::radians(m_HorizontalAngle)) * cos(glm::radians(m_VerticalAngle));
	
	float cameraSensitivity = moveComp->m_RotationSpeed;
	m_HorizontalAngle += cameraInput->mouse_x * cameraSensitivity;
	m_VerticalAngle += -cameraInput->mouse_y * cameraSensitivity;

	if (m_VerticalAngle > 89.0f)
		m_VerticalAngle = 89.0f;
	if (m_VerticalAngle < -89.0f)
		m_VerticalAngle = -89.0f;

	direction.x = cos(glm::radians(m_HorizontalAngle)) * cos(glm::radians(m_VerticalAngle));
	direction.y = -sin(glm::radians(m_VerticalAngle));
	direction.z = sin(glm::radians(m_HorizontalAngle)) * cos(glm::radians(m_VerticalAngle));
	m_Front = glm::normalize(direction);
	//########################

	cameraComp->m_View = View();
	cameraComp->m_Projection = Projection();
	cameraComp->m_Position = m_Position;
	
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
	return glm::lookAt(m_Position, m_Position + m_Front, m_UpVector);
}


const rabbitVec3f& Camera::GetPosition() const 
{
	return m_Position;
}

void Camera::setPosition(const rabbitVec3f& position_)
{
	m_Position = position_;
}


float Camera::GetFieldOfView() const 
{
	return m_FieldOfView;
}

void Camera::setFieldOfView(float fieldOfView_) {
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

