#pragma once

#include <string>

namespace TextureLoading
{
	struct TextureData
	{
		unsigned char* pData;
		int width;
		int height;
		int bpp;

		static TextureData* INVALID;
	};

	struct CubemapData
	{
		TextureData* pData[6];
	};
}

namespace TextureLoading
{
	TextureData* LoadTexture(const std::string& path, bool flipY = true);
	void FreeTexture(TextureData* textureData);

	CubemapData* LoadCubemap(const std::string& path);
	void FreeCubemap(CubemapData* cubemap);
};
