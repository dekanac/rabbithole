#include "common.h"
#include "ModelLoaderGLTF.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../Renderer.h"

#include <iostream>
#include <stdio.h>

class VulkanDevice;

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT

#include "tinygltf/tiny_gltf.h"


