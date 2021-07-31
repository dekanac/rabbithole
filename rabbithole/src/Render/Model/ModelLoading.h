#pragma once

#include <string>

#include "Core.h"

namespace ModelLoading
{
	struct TextureData
	{
		unsigned char* pData;
		int width;
		int height;
		int bpp;
	};

	struct MaterialData
	{
		float shininess = 0.0f;
		Vec3 diffuse = { 0.3,0.3,0.3 };
		Vec3 specular = { 0.5,0.5,0.5 };

		TextureData* diffuseMap = nullptr;
		TextureData* specularMap = nullptr;
	};

	struct MeshVertex
	{
		Vec3 position;
		Vec3 normal;
		Vec2 uv;

		inline static unsigned int GetStride() 
		{
			return (3 + 3 + 2) * sizeof(float);
		}
	};

	struct MeshData
	{
		MeshVertex* pVertices;
		unsigned int numVertices;

		unsigned int* pIndices;
		unsigned int numIndices;
	};

	struct ObjectData
	{
		MeshData* mesh;
		MaterialData material;
	};

	struct SceneData
	{
		ObjectData** pObjects;
		unsigned int numObjects;
	};

	SceneData* LoadScene(const char* path);
	void FreeScene(SceneData* scene);

	TextureData* LoadTexture(const ::std::string& path);
	void FreeTexture(TextureData* textureData);

	void StartAssimpLogger();
	void StopAssimpLogger();


};
