#include "Render/Vulkan/precomp.h"

#include "Model.h"

#include <stddef.h>
#include <iostream>

#include "stb_image/stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "ddsloader/dds.h"

#include "Render/Renderer.h"
#include "Render/Vulkan/VulkanDescriptors.h"
#include "Render/Vulkan/VulkanTexture.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT

#include "tinygltf/tiny_gltf.h"

glm::mat4 ConvertAiMatrixToGlmMatrix(const aiMatrix4x4& aiMatrix)
{
	// Create a glm::mat4 from the aiMatrix4x4
	glm::mat4 glmMatrix(aiMatrix.a1, aiMatrix.b1, aiMatrix.c1, aiMatrix.d1,
		aiMatrix.a2, aiMatrix.b2, aiMatrix.c2, aiMatrix.d2,
		aiMatrix.a3, aiMatrix.b3, aiMatrix.c3, aiMatrix.d3,
		aiMatrix.a4, aiMatrix.b4, aiMatrix.c4, aiMatrix.d4);

	// Transpose the matrix because glm and Assimp have different matrix conventions
	//glmMatrix = glm::transpose(glmMatrix);

	return glmMatrix;
}

std::vector<VkVertexInputBindingDescription> Vertex::GetBindingDescriptions()
{
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> Vertex::GetAttributeDescriptions()
{
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, position);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, normal);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, tangent);

	attributeDescriptions[3].binding = 0;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[3].offset = offsetof(Vertex, uv);

	return attributeDescriptions;
}

//#define OLD_WAY

VulkanglTFModel::VulkanglTFModel(Renderer* renderer, std::string filename, bool flipNormalY)
	: m_Renderer(renderer)
	, m_Path(filename)
	, m_FlipNormalY(flipNormalY)
{
	LoadModelFromFile(filename);
}

VulkanglTFModel::~VulkanglTFModel()
{
}

uint32_t VulkanglTFModel::ms_CurrentDrawId = 0;

void VulkanglTFModel::LoadModelFromFile(std::string& filename)
{
	tinygltf::Model glTFInput;
	tinygltf::TinyGLTF gltfContext;
	std::string error, warning;

	auto lastSlash = filename.find_last_of('/');
	auto lastDot = filename.find_last_of('.');

	auto name = filename.substr(lastSlash + 1, lastDot - lastSlash - 1);

	std::vector<uint32_t> indexBuffer;
	std::vector<Vertex> vertexBuffer;

#if defined(OLD_WAY)
	bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, filename);

	this->LoadImages(glTFInput);
	this->LoadMaterials(glTFInput);
	this->LoadTextures(glTFInput);
	const tinygltf::Scene& scene = glTFInput.scenes[0];
	for (size_t i = 0; i < scene.nodes.size(); i++)
	{
		const tinygltf::Node node = glTFInput.nodes[scene.nodes[i]];
		this->LoadNode(node, glTFInput, nullptr, indexBuffer, vertexBuffer);
	}
#else
	Assimp::Importer importer;
	const aiScene* aiscene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs);
	ASSERT(aiscene != nullptr, "Scene not imported, file doesn't exist!");

	this->LoadMaterials(aiscene);
	this->LoadNode(aiscene->mRootNode, aiscene, nullptr, indexBuffer, vertexBuffer);
#endif

	importer.FreeScene();

	size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
	size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
	this->m_IndexCount = static_cast<uint32_t>(indexBuffer.size());

	ResourceManager& resourceManager = m_Renderer->GetResourceManager();
	VulkanDevice& device = m_Renderer->GetVulkanDevice();

	m_VertexBuffer = resourceManager.CreateBuffer(device, BufferCreateInfo{
			.flags = {BufferUsageFlags::VertexBuffer | BufferUsageFlags::TransferSrc | BufferUsageFlags::ShaderDeviceAddress | BufferUsageFlags::AccelerationStructureBuild | BufferUsageFlags::StorageBuffer},
			.memoryAccess = {MemoryAccess::GPU},
			.size = {static_cast<uint32_t>(vertexBufferSize)},
			.name = {std::format("ModelVertexBuffer_{}", name)}
		});
	m_VertexBuffer->FillBuffer(vertexBuffer.data(), vertexBufferSize);

	m_IndexBuffer = resourceManager.CreateBuffer(device, BufferCreateInfo{
			.flags = {BufferUsageFlags::IndexBuffer | BufferUsageFlags::TransferSrc | BufferUsageFlags::ShaderDeviceAddress | BufferUsageFlags::AccelerationStructureBuild | BufferUsageFlags::StorageBuffer},
			.memoryAccess = {MemoryAccess::GPU},
			.size = {static_cast<uint32_t>(indexBufferSize)},
			.name = {std::format("ModelIndexBuffer_{}", name)}
		});
	m_IndexBuffer->FillBuffer(indexBuffer.data(), indexBufferSize);
}

void VulkanglTFModel::LoadNode(const aiNode* inputNode, const aiScene* inputScene, VulkanglTFModel::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer)
{
	VulkanglTFModel::Node node{};
	node.matrix = glm::mat4(1.0f);
	node.matrix = ConvertAiMatrixToGlmMatrix(inputNode->mTransformation);

	// Load node's children
	if (inputNode->mNumChildren > 0)
	{
		for (size_t i = 0; i < inputNode->mNumChildren; i++)
		{
			LoadNode(inputNode->mChildren[i], inputScene, &node, indexBuffer, vertexBuffer);
		}
	}

	// If the node contains mesh data, we load vertices and indices from the buffers
	// In glTF this is done via accessors and buffer views
	if (inputNode->mNumMeshes > 0)
	{
		for (uint32_t m = 0; m < inputNode->mNumMeshes; m++)
		{
			const aiMesh* mesh = inputScene->mMeshes[inputNode->mMeshes[m]];
			// Iterate through all primitives of this node's mesh
			ASSERT(mesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE, "We only support triangles for now!");

			const float* positionBuffer = nullptr;
			const float* normalsBuffer = nullptr;
			const float* texCoordsBuffer = nullptr;
			const float* tangentBuffer = nullptr;
			uint32_t vertexCount = mesh->mNumVertices;

			// Get buffer data for vertex normals
			if (mesh->HasPositions())
			{
				positionBuffer = reinterpret_cast<const float*>(mesh->mVertices);
			}
			// Get buffer data for vertex normals
			if (mesh->HasNormals())
			{
				normalsBuffer = reinterpret_cast<const float*>(mesh->mNormals);
			}
			// Get buffer data for vertex texture coordinates
			// glTF supports multiple sets, we only load the first one
			if (mesh->HasTextureCoords(0))
			{
				texCoordsBuffer = reinterpret_cast<const float*>(mesh->mTextureCoords[0]);
			}

			if (mesh->HasTangentsAndBitangents())
			{
				tangentBuffer = reinterpret_cast<const float*>(mesh->mTangents);
			}

			uint32_t firstIndex = static_cast<uint32_t>(indexBuffer.size());
			uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
			uint32_t realIndexCount = 0;

			for (size_t v = 0; v < vertexCount; v++)
			{
				Vertex vert{};
				vert.position = glm::make_vec3(&positionBuffer[v * 3]);
				vert.normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
				if (m_FlipNormalY) { vert.normal.y = -vert.normal.y; }
				vert.uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 3]) : glm::vec2(0.0f);
				vert.tangent = tangentBuffer ? glm::make_vec3(&tangentBuffer[v * 3]) : glm::vec3(0.0f);
				vertexBuffer.push_back(vert);

				//minPos = glm::min(vert.position, minPos);
				//maxPos = glm::max(vert.position, maxPos);
			}

			for (uint32_t i = 0; i < mesh->mNumFaces; i++)
			{
				const aiFace primitive = mesh->mFaces[i];

				uint32_t indexCount = primitive.mNumIndices;
				realIndexCount += indexCount;
				uint32_t* indices = primitive.mIndices;

				for (uint32_t j = 0; j < indexCount; j++)
					indexBuffer.push_back(primitive.mIndices[j] + vertexStart);

			}

			Primitive rabbitPrimitive{};
			rabbitPrimitive.firstIndex = firstIndex;
			rabbitPrimitive.indexCount = realIndexCount;
			rabbitPrimitive.materialIndex = mesh->mMaterialIndex;
			node.mesh.primitives.push_back(rabbitPrimitive);
		}
	}

	if (parent)
	{
		parent->children.push_back(node);
	}
	else
	{
		m_Nodes.push_back(node);
	}
}

void VulkanglTFModel::LoadMaterials(const aiScene* input)
{
	uint32_t numOfMaterials = input->mNumMaterials;
	m_Materials.resize(numOfMaterials);

	for (uint32_t i = 0; i < numOfMaterials; i++)
	{
		aiMaterial* currentMaterial = input->mMaterials[i];

		// Get the base color factor
		aiColor4D baseColor;
		currentMaterial->Get(AI_MATKEY_BASE_COLOR, baseColor);
		m_Materials[i].baseColorFactor = rabbitVec4f{ baseColor.r, baseColor.g, baseColor.b, baseColor.a };

		aiColor3D emissiveIntensity;
		currentMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveIntensity);
		m_Materials[i].emissiveColorAndStrenght = rabbitVec4f{ emissiveIntensity.r, emissiveIntensity.b, emissiveIntensity.g, 1.f };
		// Get base color texture index

		VulkanDevice& device = m_Renderer->GetVulkanDevice();

		uint32_t textureCount = currentMaterial->GetTextureCount(aiTextureType_BASE_COLOR);
		if (textureCount > 0)
		{
			aiString texturePath;
			currentMaterial->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath);
			auto textureAndIndex = input->GetEmbeddedTextureAndIndex(texturePath.C_Str());

			VulkanTexture* baseColorTexture = nullptr;
			// texture is embedded
			if (texturePath.length > 0 && texturePath.data[0] == '*')
			{
				 TextureLoading::TextureData* textureData = TextureLoading::LoadEmbeddedTexture(input, textureAndIndex.second);

				 baseColorTexture = Renderer::instance().GetResourceManager().CreateTexture(device, textureData, ROTextureCreateInfo{
						.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst | TextureFlags::TransferSrc},
						.format = {Format::R8G8B8A8_UNORM},
						.name = {std::format("InputTexture_{}", texturePath.C_Str())},
						.generateMips = true,
						.samplerType = SamplerType::Anisotropic,
						.addressMode = AddressMode::Repeat
					});

				 TextureLoading::FreeTexture(textureData);
			}
			// texture is extern
			else if (texturePath.length > 0)
			{
				std::string texturePathStr(texturePath.C_Str());
				std::string textureExt = texturePathStr.substr(texturePathStr.find_last_of('.') + 1);
				std::string fullPath = m_Path.substr(0, m_Path.find_last_of('/') + 1) + texturePathStr;

				// if DDS texture
				if (textureExt == "dds")
				{
					TextureLoading::TextureData* textureData = TextureLoading::LoadTextureFromDDSFile(fullPath);

					baseColorTexture = Renderer::instance().GetResourceManager().CreateTexture(device, textureData, ROTextureCreateInfo{
							.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst | TextureFlags::TransferSrc},
							.format = {Format::BC7_UNORM},
							.name = {std::format("InputTexture_{}", texturePathStr.c_str())},
							.generateMips = false,
							.samplerType = {SamplerType::Anisotropic},
							.addressMode = {AddressMode::Repeat},
							.mipCount = {textureData->mipCount}
						});

					TextureLoading::FreeTexture(textureData);
				}

				else
				{
					TextureLoading::TextureData* textureData = TextureLoading::LoadTextureFromFile(fullPath);

					baseColorTexture = Renderer::instance().GetResourceManager().CreateTexture(device, textureData, ROTextureCreateInfo{
							.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst | TextureFlags::TransferSrc},
							.format = {Format::R8G8B8A8_UNORM},
							.name = {std::format("InputTexture_{}", texturePathStr.c_str())},
							.generateMips = true,
							.samplerType = {SamplerType::Anisotropic},
							.addressMode = {AddressMode::Repeat},
						});

					TextureLoading::FreeTexture(textureData);
				}

			}

			ASSERT(baseColorTexture, "Failed to load texture");

			m_Textures.push_back(baseColorTexture);
			m_Materials[i].baseColorTextureIndex = static_cast<int32_t>(m_Textures.size()) - 1;
		}

		textureCount = currentMaterial->GetTextureCount(aiTextureType_METALNESS);
		if (textureCount > 0)
		{
			aiString texturePath;
			currentMaterial->GetTexture(aiTextureType_METALNESS, 0, &texturePath);
			auto textureAndIndex = input->GetEmbeddedTextureAndIndex(texturePath.C_Str());

			VulkanTexture* metallicRoughnessTexture = nullptr;
			// texture is embedded
			if (texturePath.length > 0 && texturePath.data[0] == '*')
			{
				TextureLoading::TextureData* textureData = TextureLoading::LoadEmbeddedTexture(input, textureAndIndex.second);

				metallicRoughnessTexture = Renderer::instance().GetResourceManager().CreateTexture(device, textureData, ROTextureCreateInfo{
					   .flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst | TextureFlags::TransferSrc},
					   .format = {Format::R8G8B8A8_UNORM},
					   .name = {std::format("InputTexture_{}", texturePath.C_Str())},
					   .generateMips = true,
					   .samplerType = SamplerType::Anisotropic,
					   .addressMode = AddressMode::Repeat
					});

				TextureLoading::FreeTexture(textureData);
			}
			// texture is extern
			else if (texturePath.length > 0)
			{
				std::string texturePathStr(texturePath.C_Str());
				std::string textureExt = texturePathStr.substr(texturePathStr.find_last_of('.') + 1);
				std::string fullPath = m_Path.substr(0, m_Path.find_last_of('/') + 1) + texturePathStr;

				if (textureExt == "dds")
				{
					TextureLoading::TextureData* textureData = TextureLoading::LoadTextureFromDDSFile(fullPath);

					metallicRoughnessTexture = Renderer::instance().GetResourceManager().CreateTexture(device, textureData, ROTextureCreateInfo{
							.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst | TextureFlags::TransferSrc},
							.format = {Format::BC7_UNORM},
							.name = {std::format("InputTexture_{}", texturePathStr.c_str())},
							.generateMips = false,
							.samplerType = {SamplerType::Anisotropic},
							.addressMode = {AddressMode::Repeat},
							.mipCount = {textureData->mipCount}
						});

					TextureLoading::FreeTexture(textureData);
				}

				else
				{
					TextureLoading::TextureData* textureData = TextureLoading::LoadTextureFromFile(fullPath);

					metallicRoughnessTexture = Renderer::instance().GetResourceManager().CreateTexture(device, textureData, ROTextureCreateInfo{
							.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst | TextureFlags::TransferSrc},
							.format = {Format::R8G8B8A8_UNORM},
							.name = {std::format("InputTexture_{}", texturePathStr.c_str())},
							.generateMips = true,
							.samplerType = {SamplerType::Anisotropic},
							.addressMode = {AddressMode::Repeat},
						});

					TextureLoading::FreeTexture(textureData);
				}

			}

			ASSERT(metallicRoughnessTexture, "Failed to load texture");

			m_Textures.push_back(metallicRoughnessTexture);
			m_Materials[i].metallicRoughnessTextureIndex = static_cast<int32_t>(m_Textures.size()) - 1;
		}

		textureCount = currentMaterial->GetTextureCount(aiTextureType_NORMALS);
		if (textureCount > 0)
		{
			aiString texturePath;
			currentMaterial->GetTexture(aiTextureType_NORMALS, 0, &texturePath);
			auto textureAndIndex = input->GetEmbeddedTextureAndIndex(texturePath.C_Str());

			VulkanTexture* normalTexture = nullptr;
			// texture is embedded
			if (texturePath.length > 0 && texturePath.data[0] == '*')
			{
				TextureLoading::TextureData* textureData = TextureLoading::LoadEmbeddedTexture(input, textureAndIndex.second);

				normalTexture = Renderer::instance().GetResourceManager().CreateTexture(device, textureData, ROTextureCreateInfo{
					   .flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst | TextureFlags::TransferSrc},
					   .format = {Format::R8G8B8A8_UNORM},
					   .name = {std::format("InputTexture_{}", texturePath.C_Str())},
					   .generateMips = true,
					   .samplerType = SamplerType::Anisotropic,
					   .addressMode = AddressMode::Repeat
					});

				TextureLoading::FreeTexture(textureData);
			}
			// texture is extern
			else if (texturePath.length > 0)
			{
				std::string texturePathStr(texturePath.C_Str());
				std::string textureExt = texturePathStr.substr(texturePathStr.find_last_of('.') + 1);
				std::string fullPath = m_Path.substr(0, m_Path.find_last_of('/') + 1) + texturePathStr;

				if (textureExt == "dds")
				{
					TextureLoading::TextureData* textureData = TextureLoading::LoadTextureFromDDSFile(fullPath);

					normalTexture = Renderer::instance().GetResourceManager().CreateTexture(device, textureData, ROTextureCreateInfo{
							.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst | TextureFlags::TransferSrc},
							.format = {Format::BC5_UNORM},
							.name = {std::format("InputTexture_{}", texturePathStr.c_str())},
							.generateMips = false,
							.samplerType = {SamplerType::Anisotropic},
							.addressMode = {AddressMode::Repeat},
							.mipCount = {textureData->mipCount}
						});

					TextureLoading::FreeTexture(textureData);
				}

				else
				{
					TextureLoading::TextureData* textureData = TextureLoading::LoadTextureFromFile(fullPath);

					normalTexture = Renderer::instance().GetResourceManager().CreateTexture(device, textureData, ROTextureCreateInfo{
							.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst | TextureFlags::TransferSrc},
							.format = {Format::R8G8B8A8_UNORM},
							.name = {std::format("InputTexture_{}", texturePathStr.c_str())},
							.generateMips = true,
							.samplerType = {SamplerType::Anisotropic},
							.addressMode = {AddressMode::Repeat},
						});

					TextureLoading::FreeTexture(textureData);
				}
			}

			ASSERT(normalTexture, "Failed to load texture");

			m_Textures.push_back(normalTexture);
			m_Materials[i].normalTextureIndex = static_cast<int32_t>(m_Textures.size()) - 1;
		}
	}
}

void VulkanglTFModel::LoadImages(tinygltf::Model& input)
{
	// Images can be stored inside the glTF (which is the case for the sample model), so instead of directly
	// loading them from disk, we fetch them from the glTF loader and upload the buffers
	m_Textures.resize(input.images.size());
	for (size_t i = 0; i < input.images.size(); i++) 
	{
		tinygltf::Image& glTFImage = input.images[i];
		// Get the image data from the glTF loader
		unsigned char* buffer = nullptr;
		VkDeviceSize bufferSize = 0;
		bool deleteBuffer = false;
		// We convert RGB-only images to RGBA, as most devices don't support RGB-formats in Vulkan
		if (glTFImage.component == 3) 
		{
			bufferSize = glTFImage.width * glTFImage.height * 4;
			buffer = new unsigned char[bufferSize];
			unsigned char* rgba = buffer;
			unsigned char* rgb = &glTFImage.image[0];
			for (size_t i = 0; i < glTFImage.width * glTFImage.height; ++i) 
			{
				memcpy(rgba, rgb, sizeof(unsigned char) * 3);
				rgba += 4;
				rgb += 3;
			}
			deleteBuffer = true;
		}
		else 
		{
			buffer = &glTFImage.image[0];
			bufferSize = glTFImage.image.size();
		}

		TextureData textureData{};
		textureData.bpp = 4;
		textureData.height = input.images[i].height;
		textureData.width = input.images[i].width;
		textureData.pData = buffer;

		ResourceManager& resourceManager = m_Renderer->GetResourceManager();
		VulkanDevice& device = m_Renderer->GetVulkanDevice();
		
		m_Textures[i] = Renderer::instance().GetResourceManager().CreateTexture(device, &textureData, ROTextureCreateInfo{
				.flags = {TextureFlags::Color | TextureFlags::Read | TextureFlags::TransferDst | TextureFlags::TransferSrc},
				.format = {Format::R8G8B8A8_UNORM},
				.name = {std::format("InputTexture_{}", glTFImage.name)},
				.generateMips = true,
				.samplerType = SamplerType::Anisotropic,
				.addressMode = AddressMode::Repeat
			});

		if (deleteBuffer) 
		{
			delete buffer;
		}
	}
}

void VulkanglTFModel::LoadTextures(tinygltf::Model& input)
{
	m_TextureIndices.resize(input.textures.size());
	for (size_t i = 0; i < input.textures.size(); i++) 
	{
		m_TextureIndices[i] = input.textures[i].source;
	}
}

void VulkanglTFModel::LoadMaterials(tinygltf::Model& input)
{
	m_Materials.resize(input.materials.size());
	for (size_t i = 0; i < input.materials.size(); i++) 
	{
		// We only read the most basic properties required for our sample
		tinygltf::Material glTFMaterial = input.materials[i];
		// Get the base color factor
		if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end()) 
		{
			m_Materials[i].baseColorFactor = glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
		}
		// Get Emissive Color and Strength
		//if (glTFMaterial.values.find("emissiveFactor") != glTFMaterial.values.end())
		{
			glm::vec3 emis = glm::make_vec3(glTFMaterial.emissiveFactor.data());
			m_Materials[i].emissiveColorAndStrenght = glm::vec4(emis.x, emis.y, emis.z, 1.f);
		}
		// Get base color texture index
		if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end()) 
		{
			m_Materials[i].baseColorTextureIndex = glTFMaterial.values["baseColorTexture"].TextureIndex();
		}
		// Get metallic and roughness texture index
		if (glTFMaterial.values.find("metallicRoughnessTexture") != glTFMaterial.values.end())
		{
			m_Materials[i].metallicRoughnessTextureIndex = glTFMaterial.values["metallicRoughnessTexture"].TextureIndex();
		}
		// Get normal texture index
		m_Materials[i].normalTextureIndex = glTFMaterial.normalTexture.index != -1 ? glTFMaterial.normalTexture.index : -1;
	}
}

void VulkanglTFModel::LoadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, VulkanglTFModel::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer)
{
	VulkanglTFModel::Node node{};
	node.matrix = glm::mat4(1.0f);

	// Get the local node matrix
	// It's either made up from translation, rotation, scale or a 4x4 matrix
	if (inputNode.translation.size() == 3)
	{
		node.matrix = glm::translate(node.matrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
	}
	if (inputNode.rotation.size() == 4)
	{
		glm::quat q = glm::make_quat(inputNode.rotation.data());
		node.matrix *= glm::mat4(q);
	}
	if (inputNode.scale.size() == 3)
	{
		node.matrix = glm::scale(node.matrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
	}
	if (inputNode.matrix.size() == 16)
	{
		node.matrix = glm::make_mat4x4(inputNode.matrix.data());
	};

	// Load node's children
	if (inputNode.children.size() > 0)
	{
		for (size_t i = 0; i < inputNode.children.size(); i++)
		{
			LoadNode(input.nodes[inputNode.children[i]], input, &node, indexBuffer, vertexBuffer);
		}
	}

	// If the node contains mesh data, we load vertices and indices from the buffers
	// In glTF this is done via accessors and buffer views
	if (inputNode.mesh > -1)
	{
		const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];
		// Iterate through all primitives of this node's mesh
		for (size_t i = 0; i < mesh.primitives.size(); i++)
		{
			const tinygltf::Primitive& glTFPrimitive = mesh.primitives[i];
			uint32_t firstIndex = static_cast<uint32_t>(indexBuffer.size());
			uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
			uint32_t indexCount = 0;
			// Vertices
			{
				const float* positionBuffer = nullptr;
				const float* normalsBuffer = nullptr;
				const float* texCoordsBuffer = nullptr;
				const float* tangentBuffer = nullptr;
				size_t vertexCount = 0;

				// Get buffer data for vertex normals
				if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					positionBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					vertexCount = accessor.count;
				}
				// Get buffer data for vertex normals
				if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					normalsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}
				// Get buffer data for vertex texture coordinates
				// glTF supports multiple sets, we only load the first one
				if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					texCoordsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}

				if (glTFPrimitive.attributes.find("TANGENT") != glTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TANGENT")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					tangentBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}

				// Append data to model's vertex buffer and calculate AABB 

				AABB aabb{};
				rabbitVec3f minPos = { FLT_MAX, FLT_MAX, FLT_MAX };
				rabbitVec3f maxPos = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

				for (size_t v = 0; v < vertexCount; v++)
				{
					Vertex vert{};
					vert.position = glm::make_vec3(&positionBuffer[v * 3]);
					vert.normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
					vert.uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec2(0.0f);
					vert.tangent = tangentBuffer ? glm::make_vec3(&tangentBuffer[v * 3]) : glm::vec3(0.0f);
					vertexBuffer.push_back(vert);

					minPos = glm::min(vert.position, minPos);
					maxPos = glm::max(vert.position, maxPos);
				}

				aabb = { minPos, maxPos };
				node.bbox = aabb;
			}
			// Indices
			{
				const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.indices];
				const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];

				indexCount += static_cast<uint32_t>(accessor.count);

				// glTF supports different component types of indices
				switch (accessor.componentType)
				{
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
				{
					uint32_t* buf = new uint32_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint32_t));
					for (size_t index = 0; index < accessor.count; index++)
					{
						indexBuffer.push_back(buf[index] + vertexStart);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
				{
					uint16_t* buf = new uint16_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint16_t));
					for (size_t index = 0; index < accessor.count; index++)
					{
						indexBuffer.push_back(buf[index] + vertexStart);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
				{
					uint8_t* buf = new uint8_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint8_t));
					for (size_t index = 0; index < accessor.count; index++)
					{
						indexBuffer.push_back(buf[index] + vertexStart);
					}
					break;
				}
				default:
					std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
					return;
				}
			}
			Primitive primitive{};
			primitive.firstIndex = firstIndex;
			primitive.indexCount = indexCount;
			primitive.materialIndex = glTFPrimitive.material;
			node.mesh.primitives.push_back(primitive);
		}
	}

	if (parent) {
		parent->children.push_back(node);
	}
	else {
		m_Nodes.push_back(node);
	}
}

void VulkanglTFModel::DrawNode(VulkanCommandBuffer& commandBuffer, const VulkanPipelineLayout* pipelineLayout, VulkanglTFModel::Node node, uint8_t backBufferIndex, IndexedIndirectBuffer* indirectBuffer)
{
	if (node.mesh.primitives.size() > 0) 
	{
		// Pass the node's matrix via push constants
		// Traverse the node hierarchy to the top-most parent to get the final matrix of the current node
		glm::mat4 nodeMatrix = node.matrix;
		VulkanglTFModel::Node* currentParent = node.parent;
		while (currentParent) 
		{
			nodeMatrix = currentParent->matrix * nodeMatrix;
			currentParent = currentParent->parent;
		}
		// Pass the final matrix to the vertex shader using push constants

		for (VulkanglTFModel::Primitive& primitive : node.mesh.primitives) 
		{
			SimplePushConstantData pushData{};
			//TODO: add primitive id
			pushData.id = ms_CurrentDrawId++;
			pushData.modelMatrix = nodeMatrix;
			pushData.useAlbedoMap = (uint32_t)(m_Materials[primitive.materialIndex].baseColorTextureIndex != -1);
			pushData.useNormalMap = (uint32_t)(m_Materials[primitive.materialIndex].normalTextureIndex != -1);
			pushData.useMetallicRoughnessMap = (uint32_t)(m_Materials[primitive.materialIndex].metallicRoughnessTextureIndex != -1);
			pushData.baseColor = m_Materials[primitive.materialIndex].baseColorFactor;
			pushData.emmisiveColorAndStrength = m_Materials[primitive.materialIndex].emissiveColorAndStrenght;

			vkCmdPushConstants(GET_VK_HANDLE(commandBuffer), GET_VK_HANDLE_PTR(pipelineLayout), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &pushData);

			//TODO: decrease num of descriptor set binding to number of different materials
			//sort primitives by materialIndexNumber
			if (primitive.indexCount > 0) 
			{
				VulkanDescriptorSet* materialDescriptorSet = m_Materials[primitive.materialIndex].materialDescriptorSet[backBufferIndex];
				// Bind the descriptor for the current primitive's texture
				vkCmdBindDescriptorSets(GET_VK_HANDLE(commandBuffer), VK_PIPELINE_BIND_POINT_GRAPHICS, GET_VK_HANDLE_PTR(pipelineLayout), 0, 1, GET_VK_HANDLE_PTR(materialDescriptorSet), 0, nullptr);

				IndexIndirectDrawData indexIndirectDrawCommand{};
                indexIndirectDrawCommand.firstIndex = primitive.firstIndex;
                indexIndirectDrawCommand.firstInstance = 0;
                indexIndirectDrawCommand.indexCount = primitive.indexCount;
                indexIndirectDrawCommand.instanceCount = 1;
                indexIndirectDrawCommand.vertexOffset = 0;

				indirectBuffer->AddIndirectDrawCommand(commandBuffer, indexIndirectDrawCommand);
			}
		}
	}
	for (auto& child : node.children) 
	{
		DrawNode(commandBuffer, pipelineLayout, child, backBufferIndex, indirectBuffer);
	}
}

void VulkanglTFModel::Draw(VulkanCommandBuffer& commandBuffer, const VulkanPipelineLayout* pipeLayout, uint8_t backBufferIndex, IndexedIndirectBuffer* indirectBuffer)
{
	for (auto& node : m_Nodes)
	{
		DrawNode(commandBuffer, pipeLayout, node, backBufferIndex, indirectBuffer);
	}
}

void VulkanglTFModel::BindBuffers(VulkanCommandBuffer& commandBuffer)
{
	VkDeviceSize offsets[1] = { 0 };
	VkBuffer vertexBuffer = GET_VK_HANDLE_PTR(m_VertexBuffer);
	vkCmdBindVertexBuffers(GET_VK_HANDLE(commandBuffer), 0, 1, &vertexBuffer, offsets);
	vkCmdBindIndexBuffer(GET_VK_HANDLE(commandBuffer), GET_VK_HANDLE_PTR(m_IndexBuffer), 0, VK_INDEX_TYPE_UINT32);
}
