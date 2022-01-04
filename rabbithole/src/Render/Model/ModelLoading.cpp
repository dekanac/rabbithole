#include "ModelLoading.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>

#include "Logger/Logger.h"

#include "stb_image/stb_image.h"

#include "Core.h"

// UTIL

#define MAT_COLOR(K,ATTR) { aiColor4D c; aiReturn ret = aiGetMaterialColor(material, K, &c); if (ret == aiReturn_SUCCESS) ATTR = GetVec4(c);  }
#define MAT_FLOAT(K,ATTR) { float f; aiReturn ret = aiGetMaterialFloat(material, K, &f); if(ret == aiReturn_SUCCESS) ATTR = f; }
#define MAT_TEX(K,ATTR) if(!ATTR) { aiString path; aiReturn ret = material->GetTexture(K, 0, &path); if(ret == aiReturn_SUCCESS) ATTR = LoadTexture(std::string(path.C_Str())); }

namespace ModelLoading
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

	void GetBoundingBoxForNode(const aiScene* scene, const aiNode* nd, aiVector3D* min, aiVector3D* max, aiMatrix4x4* trafo)
	{
		aiMatrix4x4 prev;
		unsigned int n = 0, t;

		prev = *trafo;
		aiMultiplyMatrix4(trafo, &nd->mTransformation);

		for (; n < nd->mNumMeshes; ++n)
		{
			const aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
			for (t = 0; t < mesh->mNumVertices; ++t) {

				C_STRUCT aiVector3D tmp = mesh->mVertices[t];
				aiTransformVecByMatrix4(&tmp, trafo);

				min->x = MIN(min->x, tmp.x);
				min->y = MIN(min->y, tmp.y);
				min->z = MIN(min->z, tmp.z);

				max->x = MAX(max->x, tmp.x);
				max->y = MAX(max->y, tmp.y);
				max->z = MAX(max->z, tmp.z);
			}
		}

		for (n = 0; n < nd->mNumChildren; ++n)
		{
			GetBoundingBoxForNode(scene, nd->mChildren[n], min, max, trafo);
		}
		*trafo = prev;
	}

	void GetBoundingBox(const aiScene* scene, aiVector3D* min, aiVector3D* max)
	{
		C_STRUCT aiMatrix4x4 trafo;
		aiIdentityMatrix4(&trafo);

		min->x = min->y = min->z = 1e10f;
		max->x = max->y = max->z = -1e10f;
		GetBoundingBoxForNode(scene, scene->mRootNode, min, max, &trafo);
	}

	inline Vec4 GetVec4(const aiColor4D& data) { return Vec4(data.r, data.g, data.b, data.a); }
	inline Vec3 GetVec3(const aiColor3D& data) { return Vec3(data.r, data.g, data.b); }
	inline Vec3 GetVec3(const aiVector3D& data) { return Vec3(data.x, data.y, data.z); }
	inline Vec2 GetVec2(const aiVector2D& data) { return Vec2(data.x, data.y); }

	LightType ConvertAiType(aiLightSourceType type)
	{
		switch (type)
		{
		case aiLightSource_DIRECTIONAL:
			return LightType::Directional;
		case aiLightSource_POINT:
			return LightType::Point;
		case aiLightSource_SPOT:
			return LightType::Spot;
		case aiLightSource_AMBIENT:
			return LightType::Ambient;
		case aiLightSource_AREA:
			return LightType::Area;
		//default:
			
		}
		return LightType::Directional;
	}

	void StartAssimpLogger()
	{
		aiLogStream stream;
		stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT, NULL);
		aiAttachLogStream(&stream);

		//stream = aiGetPredefinedLogStream(aiDefaultLogStream_FILE, "assimp_log.txt");
		//aiAttachLogStream(&stream);
	}

	void StopAssimpLogger()
	{
		aiDetachAllLogStreams();
	}
}

// Loading
namespace ModelLoading
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

	MaterialData LoadMaterial(aiMaterial* material)
	{
		// TODO: normal vs pbr materials

		MaterialData mat;

		// Values
		MAT_COLOR(AI_MATKEY_COLOR_DIFFUSE, mat.diffuse);

		// HACKED but ok for now
		MAT_TEX(aiTextureType_DIFFUSE, mat.diffuseMap);
		MAT_TEX(aiTextureType_NORMALS, mat.normalMap);
		MAT_TEX(aiTextureType_SPECULAR, mat.metallicMap);
		MAT_TEX(aiTextureType_AMBIENT, mat.roughnessMap);
		//MAT_TEX(aiTextureType_BASE_COLOR, mat.diffuseMap);


		//MAT_TEX(aiTextureType_DIFFUSE_ROUGHNESS, mat.roughnessMap);
		//MAT_TEX(aiTextureType_SPECULAR, mat.roughnessMap);
		//
		//MAT_TEX(aiTextureType_METALNESS, mat.metallicMap);
		//MAT_TEX(aiTextureType_REFLECTION, mat.metallicMap);
		//
		//MAT_TEX(aiTextureType_AMBIENT_OCCLUSION, mat.aoMap);
		return mat;
	}

	MeshData* LoadMesh(aiMesh* mesh)
	{
		const size_t numVertices = mesh->mNumVertices;
		const size_t numFaces = mesh->mNumFaces;
		const size_t numIndices = numFaces * 3;
		const bool loadUV = mesh->GetNumUVChannels() > 0;
		const bool loadTangents = mesh->mTangents != nullptr;
		const bool loadNormals = mesh->mNormals != nullptr;

		MeshVertex* vertices = new MeshVertex[numVertices];
		unsigned int* indices = new unsigned int[numIndices];

		for (size_t v = 0; v < mesh->mNumVertices; v++)
		{
			MeshVertex& vert = vertices[v];
			vert.position = GetVec3(mesh->mVertices[v]);

			if (loadNormals)
			{
				vert.normal = GetVec3(mesh->mNormals[v]);
			}
			else
			{
				// TODO: Calculate normal
			}

			if (loadTangents)
			{
				vert.tangent = GetVec3(mesh->mTangents[v]);
			}
			else
			{
				// TODO: Calculate tangent
			}

			if (loadUV)
			{
				Vec3 uvs = GetVec3(mesh->mTextureCoords[0][v]);
				vert.uv = Vec2(uvs.x, uvs.y);
			}
			else
			{
				vert.uv = Vec2(0.0f, 0.0f);
			}

			// TODO: multiple uv channels
		}

		for (size_t f = 0; f < numFaces; f++)
		{
			aiFace& face = mesh->mFaces[f];
			//ASSERT(face.mNumIndices == 3, "[ModelLoading] Not supported anything else except triangle meshes!");

			indices[f * 3] = face.mIndices[0];
			indices[f * 3 + 1] = face.mIndices[1];
			indices[f * 3 + 2] = face.mIndices[2];

			// TODO: if(!loadTangents) calculate manually
		}

		MeshData* meshData = new MeshData();
		meshData->pVertices = vertices;
		meshData->numVertices = numVertices;
		meshData->pIndices = indices;
		meshData->numIndices = numIndices;
		return meshData;
	}

	ObjectData* LoadObject(const aiScene* scene, unsigned meshIndex)
	{
		aiMesh* _mesh = scene->mMeshes[meshIndex];
		MeshData* mesh = LoadMesh(_mesh);

		aiMaterial* _material = scene->mMaterials[_mesh->mMaterialIndex];
		MaterialData material = LoadMaterial(_material);

		return new ObjectData{ mesh, material };
	}

	LightData* LoadLight(aiLight* light)
	{
		LightData* lightData = new LightData();
		lightData->type = ConvertAiType(light->mType);
		lightData->position = GetVec3(light->mPosition);
		lightData->direction = GetVec3(light->mDirection);
		lightData->attenuation = { light->mAttenuationConstant, light->mAttenuationLinear, light->mAttenuationQuadratic };
		lightData->color = GetVec3(light->mColorDiffuse);
		return lightData;
	}

	SceneData* LoadScene(const char* path)
	{
		s_CurrentPath = FormatPath(GetPathWitoutFile(std::string(path)));

		const aiScene* scene = nullptr;
		scene = aiImportFile(path, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded);
		if (!scene) return nullptr;

		// TODO: AABB
		//get_bounding_box(&scene_min, &scene_max);
		//scene_center.x = (scene_min.x + scene_max.x) / 2.0f;
		//scene_center.y = (scene_min.y + scene_max.y) / 2.0f;
		//scene_center.z = (scene_min.z + scene_max.z) / 2.0f;

		ObjectData** objects = new ObjectData * [scene->mNumMeshes];

		for (size_t i = 0; i < scene->mNumMeshes; i++)
		{
			ObjectData* object = LoadObject(scene, i);
			objects[i] = object;
		}

		LightData** lights = new LightData * [scene->mNumLights];

		for (size_t i = 0; i < scene->mNumLights; i++)
		{
			LightData* light = LoadLight(scene->mLights[i]);
			lights[i] = light;
		}

		SceneData* sceneData = new SceneData{ objects, scene->mNumMeshes , lights, scene->mNumLights };

		s_CurrentPath = "";
		aiReleaseImport(scene);

		return sceneData;
	}
}

// Freeing
namespace ModelLoading
{
	void FreeTexture(TextureData* textureData)
	{
		if (textureData == TextureData::INVALID) return;

		free(textureData->pData);
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

	void FreeMaterial(MaterialData& material)
	{
		if (material.diffuseMap)
			FreeTexture(material.diffuseMap);

		if (material.metallicMap)
			FreeTexture(material.metallicMap);

		if (material.roughnessMap)
			FreeTexture(material.roughnessMap);

		if (material.normalMap)
			FreeTexture(material.normalMap);
	}

	void FreeMesh(MeshData* mesh)
	{
		delete[] mesh->pVertices;
		delete[] mesh->pIndices;
		delete mesh;
	}

	void FreeObject(ObjectData* object)
	{
		FreeMesh(object->mesh);
		FreeMaterial(object->material);
		delete object;
	}

	void FreeLight(LightData* data)
	{
		delete data;
	}

	void FreeScene(SceneData* scene)
	{
		for (size_t i = 0; i < scene->numObjects; i++)
		{
			FreeObject(scene->pObjects[i]);
		}

		for (size_t i = 0; i < scene->numLights; i++)
		{
			FreeLight(scene->pLights[i]);
		}

		delete scene;
	}
};