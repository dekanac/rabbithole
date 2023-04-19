#pragma once

#include "Render/Vulkan/VulkanTypes.h"

#include <Assimp/Importer.hpp>
#include <Assimp/scene.h>
#include <Assimp/postprocess.h>

#include <string>

class VulkanTexture;

namespace TextureLoading
{
	struct TextureData
	{
		unsigned char* pData;
		int32_t width;
		int32_t height;
		int32_t bpp;
		uint32_t mipCount;

		static TextureData* INVALID;
	};

	struct CubemapData
	{
		TextureData* pData[6];
	};
}

namespace TextureLoading
{
	TextureData* LoadTextureFromFile(const std::string& path, bool flipY = true);
	TextureData* LoadTextureFromDDSFile(const std::string& path);
	TextureData* LoadEmbeddedTexture(const aiScene* scene, uint32_t index);
	CubemapData* LoadCubemap(const std::string& path);
	
	void FreeTexture(TextureData* textureData);
	void FreeCubemap(CubemapData* cubemap);
};
