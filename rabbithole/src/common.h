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


//move this to separate math lib
typedef glm::vec3 rabbitVec3f;
typedef glm::vec2 rabbitVec2f;
typedef glm::mat4 rabbitMat4f;