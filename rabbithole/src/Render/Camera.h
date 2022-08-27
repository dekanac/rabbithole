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

	const rabbitVec3f GetPosition() const;

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

struct CameraState
{
	rabbitMat4f PrevViewProjMatrix{ 1.f };
	rabbitMat4f ViewMatrix{ 1.f };
	rabbitMat4f PrevViewMatrix{ 1.f };
	rabbitMat4f ViewInverseMatrix{ 1.f };

	rabbitMat4f ProjectionMatrix{ 1.f };
	rabbitMat4f ProjectionInverseMatrix{ 1.f };

	rabbitMat4f ProjMatrixJittered{ 1.f };
	rabbitMat4f ViewProjMatrix{ 1.f };
	rabbitMat4f ViewProjInverseMatrix{ 1.f };
	rabbitVec3f CameraPosition{ 1.f };

	rabbitVec4f EyeXAxis{ 1.f };
	rabbitVec4f EyeYAxis{ 1.f };
	rabbitVec4f EyeZAxis{ 1.f };

	bool HasViewProjMatrixChanged = false;
};
