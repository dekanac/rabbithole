#pragma once

#include <glm/glm.hpp>

#define SAFE_DELETE(X) if((X)) { delete (X); (X) = nullptr; }

#define MIN(X,Y) ((X) < (Y)) ? (X) : (Y)
#define MAX(X,Y) ((X) > (Y)) ? (X) : (Y)

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;

using Mat3 = glm::mat3;
using Mat4 = glm::mat4;

class RMat4 : public Mat4 // Row major
{
public:
	RMat4():
		Mat4()
	{ }

	explicit RMat4(Mat4 mat)
	{
		for (size_t i = 0; i < 4; i++)
		{
			for (size_t j = 0; j < 4; j++)
			{
				(*this)[i][j] = mat[j][i];
			}
		}
	}
};

#define VEC2_ZERO Vec2(0.0f,0.0f)
#define VEC2_ONE Vec2(1.0f,1.0f)

#define VEC3_ZERO Vec3(0.0f,0.0f,0.0f)
#define VEC3_ONE Vec3(1.0f,1.0f,1.0f)

#define VEC4_ZERO Vec4(0.0f,0.0f,0.0f,0.0f)

#define RMAT4_IDENTITY RMat4(Mat4(1.0))
#define MAT4_IDENTITY Mat4(1.0)
#define MAT3_IDENTITY Mat3(1.0)

#define DELETE_COPY_CONSTRUCTOR(X) \
X(X const&) = delete; \
X& operator=(X const&) = delete;
/*
This is how to output to debug window
--------------------------------------------------
#include <debugapi.h>

std::wstring line = L"Mouse pos: " + std::to_wstring(dx) + L" " + std::to_wstring(dy) + L"\n";
OutputDebugString(line.c_str());

*/