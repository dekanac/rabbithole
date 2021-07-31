#include "ModelLoading.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>

#include "Logger/Logger.h"

#include "stb_image/stb_image.h"

#include "Core.h"

namespace ModelLoading
{
	namespace Private
	{
		static std::string s_CurrentPath; // TODO: Make this mtr

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
		inline Vec3 GetVec3(const aiVector3D& data) { return Vec3(data.x, data.y, data.z); }
		inline Vec2 GetVec2(const aiVector2D& data) { return Vec2(data.x, data.y); }

#define MAT_COLOR(K,ATTR) { aiColor4D c; aiReturn ret = aiGetMaterialColor(material, K, &c); if (ret == aiReturn_SUCCESS) ATTR = Private::GetVec4(c);  }
#define MAT_FLOAT(K,ATTR) { float f; aiReturn ret = aiGetMaterialFloat(material, K, &f); if(ret == aiReturn_SUCCESS) ATTR = f; }
#define MAT_TEX(K,ATTR) { aiString path; aiReturn ret = material->GetTexture(K, 0, &path); if(ret == aiReturn_SUCCESS) ATTR = LoadTexture(std::string(path.C_Str())); }

		MaterialData LoadMaterial(aiMaterial* material)
		{
			MaterialData mat;

			MAT_COLOR(AI_MATKEY_COLOR_DIFFUSE, mat.diffuse);
			MAT_COLOR(AI_MATKEY_COLOR_SPECULAR, mat.specular);
			MAT_FLOAT(AI_MATKEY_SHININESS, mat.shininess);
			MAT_TEX(aiTextureType_DIFFUSE, mat.diffuseMap);
			//MAT_TEX(aiTextureType_BASE_COLOR, mat.diffuseMap);
			MAT_TEX(aiTextureType_SPECULAR, mat.specularMap);

			// TODO: Load multiple textures if exist
			// TODO: AI_MATKEY_COLOR_EMISSIVE , AI_MATKEY_TWOSIDED , AI_MATKEY_COLOR_AMBIENT

			return mat;
		}

#undef MAT_TEX
#undef MAT_FLOAT
#undef MAT_COLOR

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
				vert.position = Private::GetVec3(mesh->mVertices[v]);

				if (loadNormals)
				{
					vert.normal = Private::GetVec3(mesh->mNormals[v]);
				}
				else
				{
					// TODO: Calculate normal
				}

				if (loadTangents)
				{
					//vert.tangent = Private::GetVec3(mesh->mTangents[v]); TODO: Tmp disabled
				}
				else
				{
					// TODO: Calculate tangent
				}

				if (loadUV)
				{
					Vec3 uvs = Private::GetVec3(mesh->mTextureCoords[0][v]);
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
				//ASSERT(face.mNumIndices == 3, "Number of indices is not 3!");

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

	static std::string GetPathWitoutFile(std::string path)
	{
		return path.substr(0, 1 + path.find_last_of("\\/"));
	}

	TextureData* LoadTexture(const std::string& path)
	{
		std::string apsolutePath = Private::s_CurrentPath + path;
		int width, height, numChannels;
		int forceNumChannels = 4;
		unsigned char* data = stbi_load(apsolutePath.c_str(), &width, &height,
			&numChannels, forceNumChannels);
		//ASSERT(data, "data is null");

		return new TextureData{ data, width, height, numChannels };
	}

	void FreeTexture(TextureData* textureData)
	{
		free(textureData->pData);
		delete textureData;
	}

	SceneData* LoadScene(const char* path)
	{
		Private::s_CurrentPath = GetPathWitoutFile(std::string(path));

		const aiScene* scene = nullptr;
		scene = aiImportFile(path, aiProcessPreset_TargetRealtime_MaxQuality);
		if (!scene) return nullptr;

		// TODO: AABB
		//get_bounding_box(&scene_min, &scene_max);
		//scene_center.x = (scene_min.x + scene_max.x) / 2.0f;
		//scene_center.y = (scene_min.y + scene_max.y) / 2.0f;
		//scene_center.z = (scene_min.z + scene_max.z) / 2.0f;

		ObjectData** objects = new ObjectData*[scene->mNumMeshes];

		for (size_t i = 0; i < scene->mNumMeshes; i++)
		{
			ObjectData* object = Private::LoadObject(scene, i);
			objects[i] = object;
		}

		SceneData* sceneData = new SceneData{ objects, scene->mNumMeshes };

		aiReleaseImport(scene);

		return sceneData;
	}

	void FreeMesh(MeshData* mesh)
	{
		delete[] mesh->pVertices;
		delete[] mesh->pIndices;
		delete mesh;
	}

	void FreeMaterial(MaterialData& material)
	{
		if(material.diffuseMap)
			FreeTexture(material.diffuseMap);
		if(material.specularMap)
			FreeTexture(material.specularMap);
	}

	void FreeObect(ObjectData* object)
	{
		FreeMesh(object->mesh);
		FreeMaterial(object->material);
		delete object;
	}

	void FreeScene(SceneData* scene)
	{
		for (size_t i = 0; i < scene->numObjects; i++)
		{
			FreeObect(scene->pObjects[i]);
		}

		delete scene;
	}
};