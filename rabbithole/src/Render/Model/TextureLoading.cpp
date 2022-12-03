#include "TextureLoading.h"

#include <string>

#include "Logger/Logger.h"
#include "Utils/utils.h"

#include "stb_image/stb_image.h"

namespace TextureLoading
{
	TextureData* TextureData::INVALID = nullptr;
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
}

// Loading
namespace TextureLoading
{
	TextureData* LoadTexture(const std::string& path, bool flipY)
	{
		std::string apsolutePath = GetAbsolutePath(path);
		int width, height, numChannels;
		int forceNumChannels = 4;
		unsigned char* data = stbi_load(apsolutePath.c_str(), &width, &height,
			&numChannels, forceNumChannels);

		if (!data)
		{
			// TODO: Move this on some init function and delete if at the engine stop
			if (!TextureData::INVALID)
			{
				unsigned char invalid_color[] = { 0xff, 0x00, 0x33, 0xff };
				TextureData::INVALID = new TextureData{ invalid_color, 1, 1, 4 };
			}
			return TextureData::INVALID;
		}

		return new TextureData{ data, width, height, numChannels };
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
			output->pData[i] = LoadTexture(imagePath, false);
			ASSERT(i == 0 || (last_height == output->pData[i]->height && last_width == output->pData[i]->width), "[LoadCubemap] Invalid cubemap textures format!");
			last_height = output->pData[i]->height;
			last_width = output->pData[i]->width;
		}

		return output;
	}
}

// Freeing
namespace TextureLoading
{
	void FreeTexture(TextureData* textureData)
	{
		if (textureData == TextureData::INVALID) return;

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

};