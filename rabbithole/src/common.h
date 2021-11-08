#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#pragma once

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
    ~className() = default;

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

//move this to separate math lib
typedef glm::vec3 rabbitVec3f;
typedef glm::vec2 rabbitVec2f;
typedef glm::mat4 rabbitMat4f;

#define ZERO_VEC3f rabbitVec3f{0.0, 0.0, 0.0}
#define ZERO_MAT4f rabbitMat4f{1.f}

#define MB_16 16777216
