#pragma once
#include "common.h"

#include <glm/glm.hpp>

class EntityManager;

class Camera
{

private:
	Vector3f m_Position;
	float m_FieldOfView;
	float m_NearPlane;
	float m_FarPlane;
	float m_Aspect;
	float m_Pitch;
	float m_Yaw;
	Matrix44f m_ProjMatrix;
	Matrix44f m_ProjMatrixJittered;
	Matrix44f m_ViewMatrix;
	Vector3f m_FocalPoint;
	float m_CameraPanSpeed;
	float m_Distance;

public:
	Camera();

	bool Init();
	void Update(float dt);

	const Vector3f GetPosition() const;

	float	GetFieldOfView() const;
	float	GetFieldOfViewVerticalRad() const;
	float	GetNearPlane() const;
	float	GetFarPlane() const;
	void	SetFieldOfView(float fieldOfView);
	void	SetNearAndFarPlanes(float nearPlane, float farPlane);
	void	SetViewportAspectRatio(float vpaspect);

	Quaternion GetOrientation() const;
	Vector3f GetUpDirection() const;
	Vector3f CalculatePosition() const;
	Vector3f GetRightDirection() const;
	Vector3f GetForwardDirection() const;
	void UpdateView();
	void MousePan(const Vector2f& delta);
	void MouseRotate(const Vector2f& delta);
	void MouseZoom(float delta);
	void SetProjectionJitter(float jitterX, float jitterY);
	void SetProjectionJitter(uint32_t width, uint32_t height, uint32_t& sampleIndex);
	Matrix44f GetMatrix() const;
	Matrix44f Projection() const;
	Matrix44f ProjectionJittered() const;
	Matrix44f View() const;
};

struct CameraState
{
	Matrix44f PrevViewProjMatrix{ 1.f };
	Matrix44f ViewMatrix{ 1.f };
	Matrix44f PrevViewMatrix{ 1.f };
	Matrix44f ViewInverseMatrix{ 1.f };

	Matrix44f ProjectionMatrix{ 1.f };
	Matrix44f ProjectionInverseMatrix{ 1.f };

	Matrix44f ProjMatrixJittered{ 1.f };
	Matrix44f ViewProjMatrix{ 1.f };
	Matrix44f ViewProjInverseMatrix{ 1.f };
	Vector3f CameraPosition{ 1.f };

	Vector4f EyeXAxis{ 1.f };
	Vector4f EyeYAxis{ 1.f };
	Vector4f EyeZAxis{ 1.f };

	bool HasViewProjMatrixChanged = false;
};
