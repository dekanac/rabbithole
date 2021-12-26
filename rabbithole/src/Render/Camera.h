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
	rabbitMat4f m_ViewMatrix;
	glm::vec3 m_FocalPoint;
	float m_Distance;

public:
	Camera();

	bool Init();
	void Update(float dt);

	const rabbitVec3f& GetPosition() const;
	const rabbitVec3f& getFront() const;
	const rabbitVec3f& getUpVector() const;
	void setPosition(const rabbitVec3f& position_);
	void setFront(rabbitVec3f front_);
	void setUpVector(rabbitVec3f upvec_);

	float GetFieldOfView() const;
	float GetNearPlane() const;
	float GetfarPlane() const;
	void setFieldOfView(float fieldOfView_);
	void setNearAndFarPlanes(float nearPlane_, float farPlane_);
	void setViewportAspectRatio(float vpaspect_);

	glm::quat GetOrientation() const;
	glm::vec3 GetUpDirection() const;
	glm::vec3 CalculatePosition() const;
	glm::vec3 GetRightDirection() const;
	glm::vec3 GetForwardDirection() const;
	void UpdateView();
	void MousePan(const glm::vec2& delta);
	void MouseRotate(const glm::vec2& delta);
	void MouseZoom(float delta);
	rabbitMat4f GetMatrix() const;
	rabbitMat4f Projection() const;
	rabbitMat4f View() const;
};
