#pragma once
#include "common.h"

#include <glm/glm.hpp>

class EntityManager;

class Camera
{

private:
	rabbitVec3f m_Position;
	float m_FieldOfView;
	float m_NearPlane;
	float m_FarPlane;
	float m_Aspect;
	float m_Pitch;
	float m_Yaw;
	rabbitMat4f m_ProjMatrix;
	rabbitMat4f m_ProjMatrixJittered;
	rabbitMat4f m_ViewMatrix;
	rabbitVec3f m_FocalPoint;
	float m_CameraPanSpeed;
	float m_Distance;

public:
	Camera();

	bool Init();
	void Update(float dt);

	const rabbitVec3f& GetPosition() const;
	const rabbitVec3f& GetFront() const;
	const rabbitVec3f& GetUpVector() const;

	float	GetFieldOfView() const;
	float	GetFieldOfViewVerticalRad() const;
	float	GetNearPlane() const;
	float	GetFarPlane() const;
	void	SetFieldOfView(float fieldOfView);
	void	SetNearAndFarPlanes(float nearPlane, float farPlane);
	void	SetViewportAspectRatio(float vpaspect);

	glm::quat GetOrientation() const;
	glm::vec3 GetUpDirection() const;
	glm::vec3 CalculatePosition() const;
	glm::vec3 GetRightDirection() const;
	glm::vec3 GetForwardDirection() const;
	void UpdateView();
	void MousePan(const glm::vec2& delta);
	void MouseRotate(const glm::vec2& delta);
	void MouseZoom(float delta);
	void SetProjectionJitter(float jitterX, float jitterY);
	void SetProjectionJitter(uint32_t width, uint32_t height, uint32_t& sampleIndex);
	rabbitMat4f GetMatrix() const;
	rabbitMat4f Projection() const;
	rabbitMat4f ProjectionJittered() const;
	rabbitMat4f View() const;
};
