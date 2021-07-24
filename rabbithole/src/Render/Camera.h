#pragma once
#include "common.h"

#include <glm/glm.hpp>

class EntityManager;

class Camera
{

private:
	rabbitVec3f m_Position;
	rabbitVec3f m_Front;
	rabbitVec3f m_UpVector;
	float m_HorizontalAngle;
	float m_VerticalAngle;
	float m_FieldOfView;
	float m_NearPlane;
	float m_FarPlane;
	float m_Aspect;

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

	rabbitMat4f GetMatrix() const;
	rabbitMat4f Projection() const;
	rabbitMat4f View() const;
};
