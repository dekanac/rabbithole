#pragma once

#include "Render/Vulkan/VulkanTypes.h"

#include <Assimp/Importer.hpp>
#include <Assimp/scene.h>
#include <Assimp/postprocess.h>

#include <string>

namespace TextureLoading
{
	struct TextureData
	{
		unsigned char* pData = nullptr;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t size = 0;
		uint32_t mipCount = 1;
		Format format = Format::UNDEFINED;
	};

	struct CubemapData
	{
		TextureData* pData[6];
	};

	TextureData* LoadTextureFromFile(const std::string& path, bool flipY = true);
	TextureData* LoadEmbeddedTexture(const aiScene* scene, uint32_t index);
	CubemapData* LoadCubemap(const std::string& path);
	
	void FreeTexture(TextureData* textureData);
	void FreeCubemap(CubemapData* cubemap);
};
