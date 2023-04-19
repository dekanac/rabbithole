#pragma once

#include <glm/glm.hpp>

#include <limits>

#define VULKAN_HWRT

#ifdef _DEBUG
	#define RABBITHOLE_DEBUG
#endif

#define PROFILE_CODE(code) auto start = Utils::SetStartTime(); \
	code \
	Utils::SetEndtimeAndPrint(start);

#define SingletonClass(className) \
public: \
    className(className const&)         = delete; \
    className& operator=(className const&)    = delete; \
    static className& instance() { \
        static className instance; \
        return instance; \
    } \
private: \
    className() = default; \
    ~className() = default

#define NonCopyableAndMovable(className) \
public: \
	className(const className&) = delete; \
	className& operator= (const className&) = delete; \
	className(className&&) = delete; \
	className& operator= (className&&) = delete

#define VULKAN_API_CALL(call) \
		{ \
			VkResult result = call; \
			ASSERT(result == VK_SUCCESS, "Vulkan API call fail!"); \
		}

#define VULKAN_SPIRV_CALL(call) \
		{ \
			SpvReflectResult result = call; \
			ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "Vulkan Spir-V reflect call fail"); \
		}

#define GET_VK_HANDLE(object) (object.GetVkHandle())
#define GET_VK_HANDLE_PTR(object) (object->GetVkHandle())

#define GET_MIP_LEVELS_FROM_RES(width, height) (static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1)
//move this to separate math lib
typedef glm::vec4 rabbitVec4f;
typedef glm::vec3 rabbitVec3f;
typedef glm::vec2 rabbitVec2f;
typedef glm::mat4 rabbitMat4f;

#define ZERO_VEC3f rabbitVec3f{0.0, 0.0, 0.0}
#define ZERO_MAT4f rabbitMat4f{1.f}

#define YELLOW_COLOR	rabbitVec4f{1.f, 1.f, 0.f, 1.f}
#define RED_COLOR		rabbitVec4f{1.f, 0.f, 0.f, 1.f}
#define BLUE_COLOR		rabbitVec4f{0.f, 0.f, 1.f, 1.f}
#define PUPRPLE_COLOR	rabbitVec4f{1.f, 0.f, 1.f, 1.f}
#define GREEN_COLOR		rabbitVec4f{0.f, 1.f, 0.f, 1.f}
#define BLACK_COLOR		rabbitVec4f{0.f, 0.f, 0.f, 1.f}

#define MB_16 16777216
#define MB_64 67108864

#define MAX_FRAMES_IN_FLIGHT (2)

#define GetCSDispatchCount(A, B) (((A) + ((B) - 1)) / (B))

const double Epsilon = 1e-8;
const float Infinity = std::numeric_limits<float>::max();

struct WindowExtent
{
	uint32_t width;
	uint32_t height;
};

#define EXECUTE_ONCE(code) \
	static bool doOnceFlag = false; \
	if (!doOnceFlag) \
	{ \
		code; \
		doOnceFlag = true; \
	}

#define RABBIT_ALLOC(t,n) (t*)Utils::RabbitMalloc((n)*sizeof(t))
#define RABBIT_FREE(ptr) Utils::RabbitFree(ptr)