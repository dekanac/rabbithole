#include "TextureLoading.h"

#include "Logger/Logger.h"
#include "Render/Converters.h"
#include "Utils/Utils.h"

#include <stb_image/stb_image.h>
#include <ddsloader/dds.h>

#include <string>

namespace TextureLoading
{
	static std::string s_CurrentPath = ""; // TODO: Make this mtr

	std::string FormatPath(const std::string& path)
	{
		std::string formatedPath = path;
		for (size_t i = 0; i < path.size(); i++)
		{
			if (formatedPath[i] == '\\')
			{
				formatedPath[i] = '/';
			}
		}
		return formatedPath;
	}

	std::string GetAbsolutePath(const std::string& relativePath)
	{
		std::string formatedPath = FormatPath(relativePath);
		return s_CurrentPath + formatedPath;
	}

	std::string GetPathWitoutFile(std::string path)
	{
		return path.substr(0, 1 + path.find_last_of("\\/"));
	}

	TextureData* LoadTextureFromDDSFile(const std::string& path)
	{
		DDSFile* dds = dds_load(path.c_str());

		TextureData* textureData = new TextureData();
		textureData->size = static_cast<uint32_t>(
			GetTexelSizeFrom(GetFormatFrom(dds->ddsHeaderDx10->dxgiFormat))
			* dds->dwHeight
			* dds->dwWidth);
		textureData->height = dds->dwHeight;
		textureData->width = dds->dwWidth;
		textureData->pData = dds->blBuffer;
		textureData->mipCount = dds->dwMipMapCount;
		textureData->format = GetFormatFrom(dds->ddsHeaderDx10->dxgiFormat);
		dds_free(dds);

		return textureData;
	}

	TextureData* LoadTextureFromFile(const std::string& path, bool flipY)
	{
		if (path.substr(path.find_last_of('.') + 1) == "dds")
			return LoadTextureFromDDSFile(path);

		std::string apsolutePath = GetAbsolutePath(path);
		int width, height, numChannels;
		int forceNumChannels = 4;
		unsigned char* data = stbi_load(apsolutePath.c_str(), &width, &height,
			&numChannels, forceNumChannels);

		ASSERT(data, "Texture loading failed!");

		return new TextureData{
			.pData = data,
			.width = static_cast<uint32_t>(width),
			.height = static_cast<uint32_t>(height),
			.size = static_cast<uint32_t>(width * height * forceNumChannels),
			.format = Format::R8G8B8A8_UNORM
		};
	}

	CubemapData* LoadCubemap(const std::string& path)
	{
		// The order is important because in that order are textures uploaded on cubemap
		static const std::string sides[] = { "R","L","U","D","B","F" };

		const size_t extBegin = path.find_last_of(".");
		ASSERT(extBegin != std::string::npos, "[LoadCubemap] Invalid path");
		const std::string ext = path.substr(extBegin + 1);
		const std::string basePath = path.substr(0, extBegin);

		CubemapData* output = new CubemapData();
		int last_width = -1, last_height = -1;
		for (int i = 0; i < 6; i++)
		{
			const std::string imagePath = basePath + "_" + sides[i] + "." + ext;
			output->pData[i] = LoadTextureFromFile(imagePath, false);
			ASSERT(i == 0 || (last_height == output->pData[i]->height && last_width == output->pData[i]->width), "[LoadCubemap] Invalid cubemap textures format!");
			last_height = output->pData[i]->height;
			last_width = output->pData[i]->width;
		}

		return output;
	}

	TextureData* LoadEmbeddedTexture(const aiScene* scene, uint32_t index)
	{
		const unsigned char* data = reinterpret_cast<const unsigned char*>(scene->mTextures[index]->pcData);
		int width, height, channels;
		unsigned char* image = stbi_load_from_memory(data, scene->mTextures[index]->mWidth, &width, &height, &channels, STBI_rgb_alpha);

		return new TextureData{
			.pData = image,
			.width = static_cast<uint32_t>(width),
			.height = static_cast<uint32_t>(height),
			.size = static_cast<uint32_t>(width * height * channels),
			.format = Format::R8G8B8A8_UNORM
		};
	}

	void FreeTexture(TextureData* textureData)
	{
		RABBIT_FREE(textureData->pData);
		delete textureData;
	}

	void FreeCubemap(CubemapData* cubemap)
	{
		for (size_t i = 0; i < 6; i++)
		{
			FreeTexture(cubemap->pData[i]);
		}
		delete cubemap;
	}

}