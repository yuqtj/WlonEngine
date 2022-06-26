
#define TINYGLTF_IMPLEMENTATION

#include "ModelReader/GltfReader.h"
#include "Scene/Light.h"
#include "Framework/GlmCommon.h"
#include "Math/MathConstant.h"

#include "Scene/GameObjectUntil.h"
#include "Scene/Mesh.h"
#include "Scene/Camera.h"
#include "Scene/Transform.h"
#include "Scene/MeshRenderer.h"
#include "Scene/Scene.h"

#include "Render/Material.h"
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <algorithm>
#include <unordered_map>
#include <queue>

#define KHR_LIGHTS_PUNCTUAL_EXTENSION "KHR_lights_punctual"

std::unique_ptr<Material> ParseMaterial(const tinygltf::Material& gltf_material)
{
	auto material = std::make_unique<Material>(gltf_material.name);

	for (auto& gltf_value : gltf_material.values)
	{
		if (gltf_value.first == "baseColorFactor")
		{
			const auto& color_factor = gltf_value.second.ColorFactor();
			material->baseColorFactor = glm::vec4(color_factor[0], color_factor[1], color_factor[2], color_factor[3]);
		}
		else if (gltf_value.first == "metallicFactor")
		{
			material->metallicFactor = static_cast<float>(gltf_value.second.Factor());
		}
		else if (gltf_value.first == "roughnessFactor")
		{
			material->roughnessFactor = static_cast<float>(gltf_value.second.Factor());
		}
	}

	for (auto& gltf_value : gltf_material.additionalValues)
	{
		if (gltf_value.first == "emissiveFactor")
		{
			const auto& emissive_factor = gltf_value.second.number_array;

			material->emissive = glm::vec3(emissive_factor[0], emissive_factor[1], emissive_factor[2]);
		}
		else if (gltf_value.first == "alphaMode")
		{
			if (gltf_value.second.string_value == "BLEND")
			{
				material->alphaMode = AlphaMode::Blend;
			}
			else if (gltf_value.second.string_value == "OPAQUE")
			{
				material->alphaMode = AlphaMode::Opaque;
			}
			else if (gltf_value.second.string_value == "MASK")
			{
				material->alphaMode = AlphaMode::Mask;
			}
		}
		else if (gltf_value.first == "alphaCutoff")
		{
			material->alphaCutoff = static_cast<float>(gltf_value.second.number_value);
		}
		else if (gltf_value.first == "doubleSided")
		{
			material->doubleSided = gltf_value.second.bool_value;
		}
	}

	return material;
}

std::unique_ptr<Material> CreateDefaultMaterial()
{
	tinygltf::Material gltf_material;
	return ParseMaterial(gltf_material);
}

std::unique_ptr<Material> defaultMaterial = CreateDefaultMaterial();

/**
 * @brief Helper Function to change array type T to array type Y
 * Create a struct that can be used with std::transform so that we do not need to recreate lambda functions
 * @param T
 * @param Y
 */
template <class T, class Y>
struct TypeCast
{
	Y operator()(T value) const noexcept
	{
		return static_cast<Y>(value);
	}
};

std::unordered_map<std::string, bool> supportedExtensions = {
	{KHR_LIGHTS_PUNCTUAL_EXTENSION, false} };

inline std::vector<uint8_t> GetAttributeData(const tinygltf::Model* model, uint32_t accessorId)
{
	auto& accessor = model->accessors.at(accessorId);
	auto& bufferView = model->bufferViews.at(accessor.bufferView);
	auto& buffer = model->buffers.at(bufferView.buffer);

	size_t stride = accessor.ByteStride(bufferView);
	size_t startByte = accessor.byteOffset + bufferView.byteOffset;
	size_t endByte = startByte + accessor.count * stride;

	return { buffer.data.begin() + startByte, buffer.data.begin() + endByte };
};

inline size_t GetAttributeSize(const tinygltf::Model* model, uint32_t accessorId)
{
	return model->accessors.at(accessorId).count;
};

inline size_t GetAttributeStride(const tinygltf::Model* model, uint32_t accessorId)
{
	auto& accessor = model->accessors.at(accessorId);
	auto& bufferView = model->bufferViews.at(accessor.bufferView);

	return accessor.ByteStride(bufferView);
};

inline VkFormat GetAttributeFormat(const tinygltf::Model* model, uint32_t accessorId)
{
	auto& accessor = model->accessors.at(accessorId);

	VkFormat format;

	switch (accessor.componentType)
	{
	case TINYGLTF_COMPONENT_TYPE_BYTE: {
		static const std::map<int, VkFormat> mapped_format = { {TINYGLTF_TYPE_SCALAR, VK_FORMAT_R8_SINT},
															  {TINYGLTF_TYPE_VEC2, VK_FORMAT_R8G8_SINT},
															  {TINYGLTF_TYPE_VEC3, VK_FORMAT_R8G8B8_SINT},
															  {TINYGLTF_TYPE_VEC4, VK_FORMAT_R8G8B8A8_SINT} };

		format = mapped_format.at(accessor.type);

		break;
	}
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
		static const std::map<int, VkFormat> mapped_format = { {TINYGLTF_TYPE_SCALAR, VK_FORMAT_R8_UINT},
															  {TINYGLTF_TYPE_VEC2, VK_FORMAT_R8G8_UINT},
															  {TINYGLTF_TYPE_VEC3, VK_FORMAT_R8G8B8_UINT},
															  {TINYGLTF_TYPE_VEC4, VK_FORMAT_R8G8B8A8_UINT} };

		static const std::map<int, VkFormat> mapped_format_normalize = { {TINYGLTF_TYPE_SCALAR, VK_FORMAT_R8_UNORM},
																		{TINYGLTF_TYPE_VEC2, VK_FORMAT_R8G8_UNORM},
																		{TINYGLTF_TYPE_VEC3, VK_FORMAT_R8G8B8_UNORM},
																		{TINYGLTF_TYPE_VEC4, VK_FORMAT_R8G8B8A8_UNORM} };

		if (accessor.normalized)
		{
			format = mapped_format_normalize.at(accessor.type);
		}
		else
		{
			format = mapped_format.at(accessor.type);
		}

		break;
	}
	case TINYGLTF_COMPONENT_TYPE_SHORT: {
		static const std::map<int, VkFormat> mapped_format = { {TINYGLTF_TYPE_SCALAR, VK_FORMAT_R8_SINT},
															  {TINYGLTF_TYPE_VEC2, VK_FORMAT_R8G8_SINT},
															  {TINYGLTF_TYPE_VEC3, VK_FORMAT_R8G8B8_SINT},
															  {TINYGLTF_TYPE_VEC4, VK_FORMAT_R8G8B8A8_SINT} };

		format = mapped_format.at(accessor.type);

		break;
	}
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
		static const std::map<int, VkFormat> mapped_format = { {TINYGLTF_TYPE_SCALAR, VK_FORMAT_R16_UINT},
															  {TINYGLTF_TYPE_VEC2, VK_FORMAT_R16G16_UINT},
															  {TINYGLTF_TYPE_VEC3, VK_FORMAT_R16G16B16_UINT},
															  {TINYGLTF_TYPE_VEC4, VK_FORMAT_R16G16B16A16_UINT} };

		static const std::map<int, VkFormat> mapped_format_normalize = { {TINYGLTF_TYPE_SCALAR, VK_FORMAT_R16_UNORM},
																		{TINYGLTF_TYPE_VEC2, VK_FORMAT_R16G16_UNORM},
																		{TINYGLTF_TYPE_VEC3, VK_FORMAT_R16G16B16_UNORM},
																		{TINYGLTF_TYPE_VEC4, VK_FORMAT_R16G16B16A16_UNORM} };

		if (accessor.normalized)
		{
			format = mapped_format_normalize.at(accessor.type);
		}
		else
		{
			format = mapped_format.at(accessor.type);
		}

		break;
	}
	case TINYGLTF_COMPONENT_TYPE_INT: {
		static const std::map<int, VkFormat> mapped_format = { {TINYGLTF_TYPE_SCALAR, VK_FORMAT_R32_SINT},
															  {TINYGLTF_TYPE_VEC2, VK_FORMAT_R32G32_SINT},
															  {TINYGLTF_TYPE_VEC3, VK_FORMAT_R32G32B32_SINT},
															  {TINYGLTF_TYPE_VEC4, VK_FORMAT_R32G32B32A32_SINT} };

		format = mapped_format.at(accessor.type);

		break;
	}
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
		static const std::map<int, VkFormat> mapped_format = { {TINYGLTF_TYPE_SCALAR, VK_FORMAT_R32_UINT},
															  {TINYGLTF_TYPE_VEC2, VK_FORMAT_R32G32_UINT},
															  {TINYGLTF_TYPE_VEC3, VK_FORMAT_R32G32B32_UINT},
															  {TINYGLTF_TYPE_VEC4, VK_FORMAT_R32G32B32A32_UINT} };

		format = mapped_format.at(accessor.type);

		break;
	}
	case TINYGLTF_COMPONENT_TYPE_FLOAT: {
		static const std::map<int, VkFormat> mapped_format = { {TINYGLTF_TYPE_SCALAR, VK_FORMAT_R32_SFLOAT},
															  {TINYGLTF_TYPE_VEC2, VK_FORMAT_R32G32_SFLOAT},
															  {TINYGLTF_TYPE_VEC3, VK_FORMAT_R32G32B32_SFLOAT},
															  {TINYGLTF_TYPE_VEC4, VK_FORMAT_R32G32B32A32_SFLOAT} };

		format = mapped_format.at(accessor.type);

		break;
	}
	default: {
		format = VK_FORMAT_UNDEFINED;
		break;
	}
	}

	return format;
};

void ParseCamera(const tinygltf::Camera& gltf_camera, GameObject* go)
{
	Camera* camera = go->AddComponent<Camera>();

	if (gltf_camera.type == "perspective")
	{
		camera->SetAspectRatio(static_cast<float>(gltf_camera.perspective.aspectRatio));
		camera->SetFieldOfView(static_cast<float>(gltf_camera.perspective.yfov));
		camera->SetNearPlane(static_cast<float>(gltf_camera.perspective.znear));
		camera->SetFarPlane(static_cast<float>(gltf_camera.perspective.zfar));
	}
	else
	{
		//LOGW("Camera type not supported");
	}
}

void ParseMesh(tinygltf::Model& model, const tinygltf::Mesh& gltfMesh, GameObject* go)
{
	Mesh* mesh = WL_NEW(Mesh);
	MeshRenderer* meshRenderer = go->AddComponent<MeshRenderer>();
	meshRenderer->SetMesh(mesh);
	for (auto& gltfPrimitive : gltfMesh.primitives)
	{
		SubMesh subMesh;
		for (auto& attribute : gltfPrimitive.attributes)
		{
			std::string attributeName = attribute.first;
			std::transform(attributeName.begin(), attributeName.end(), attributeName.begin(), ::tolower);

			uint32_t accessorId = attribute.second;
			std::vector<uint8_t> vertexData = GetAttributeData(&model, attribute.second);
			if (attributeName == "position")
			{
				subMesh.vertexCount = model.accessors.at(attribute.second).count;
			}
			subMesh.vertexBuffers.insert(std::make_pair(attributeName, std::move(vertexData)));

			VertexAttribute attrib;
			attrib.format = GetAttributeFormat(&model, attribute.second);
			attrib.stride = GetAttributeStride(&model, attribute.second);
			subMesh.SetAttribute(attributeName, attrib);
		}

		if (gltfPrimitive.indices >= 0)
		{
			subMesh.vertexIndices = GetAttributeSize(&model, gltfPrimitive.indices);

			auto format = GetAttributeFormat(&model, gltfPrimitive.indices);
			auto indexData = GetAttributeData(&model, gltfPrimitive.indices);

			switch (format)
			{
			case VK_FORMAT_R8_UINT:
				// Converts uint8 data into uint16 data, still represented by a uint8 vector
				//indexData = convert_underlying_data_stride(index_data, 1, 2);
				//submesh->index_type = VK_INDEX_TYPE_UINT16;

				break;
			case VK_FORMAT_R16_UINT:
				subMesh.indexType = VK_INDEX_TYPE_UINT16;
				break;
			case VK_FORMAT_R32_UINT:
				subMesh.indexType = VK_INDEX_TYPE_UINT32;
				break;
			default:
				break;
			}
		}

		if (gltfPrimitive.material < 0)
		{
			subMesh.material = &(*defaultMaterial);
		}
		else
		{
			subMesh.material = &(*defaultMaterial);
		}

		mesh->AddSubmesh(subMesh);
	}
}

GameObject* ParseNode(const tinygltf::Node& gltf_node, size_t index)
{
	auto node = WL_NEW(GameObject)(gltf_node.name);

	auto transform = node->AddComponent<Transform>();

	if (!gltf_node.translation.empty())
	{
		glm::vec3 translation;

		std::transform(gltf_node.translation.begin(), gltf_node.translation.end(), glm::value_ptr(translation), TypeCast<double, float>{});

		transform->SetTranslation(translation);
	}

	if (!gltf_node.rotation.empty())
	{
		glm::quat rotation;

		std::transform(gltf_node.rotation.begin(), gltf_node.rotation.end(), glm::value_ptr(rotation), TypeCast<double, float>{});

		transform->SetRotation(rotation);
	}

	if (!gltf_node.scale.empty())
	{
		glm::vec3 scale;

		std::transform(gltf_node.scale.begin(), gltf_node.scale.end(), glm::value_ptr(scale), TypeCast<double, float>{});

		transform->SetScale(scale);
	}

	if (!gltf_node.matrix.empty())
	{
		glm::mat4 matrix;

		std::transform(gltf_node.matrix.begin(), gltf_node.matrix.end(), glm::value_ptr(matrix), TypeCast<double, float>{});

		transform->SetMatrix(matrix);
	}

	return node;
}

Scene* GltfReader::LoadFile(const char* path)
{
	Scene* scene = WL_NEW(Scene);

	int scene_index = -1;

	std::string err;
	std::string warn;

	tinygltf::TinyGLTF gltfLoader;

	tinygltf::Model model;
	bool importResult = gltfLoader.LoadASCIIFromFile(&model, &err, &warn, path);
	if (!importResult)
	{
		return nullptr;
	}

	if (!err.empty())
	{
		return nullptr;
	}

	if (!warn.empty())
	{
		return nullptr;
	}

	//std::string fileName = path;
	//size_t pos = fileName.find_last_of('/');

	for (auto& usedExtension: model.extensionsUsed)
	{
		auto it = supportedExtensions.find(usedExtension);
		if (it == supportedExtensions.end())
		{
			if (std::find(model.extensionsRequired.begin(), model.extensionsRequired.end(), usedExtension) != model.extensionsRequired.end())
			{
				throw std::runtime_error("Cannot load glTF file. Contains a required unsupported extension: " + usedExtension);
			}
			else
			{

			}
		}
		else
		{
			it->second = true;
		}
	}

	//LoadLight(model);
	
	std::vector<GameObject*> nodes;
	for (size_t node_index = 0; node_index < model.nodes.size(); ++node_index)
	{
		auto gltfNode = model.nodes[node_index];
		auto node = ParseNode(gltfNode, node_index);

		if (gltfNode.mesh >= 0)
		{
			auto& mesh = model.meshes[gltfNode.mesh];
			ParseMesh(model, mesh, &(*node));
		}

		if (gltfNode.camera >= 0)
		{
			auto& camera = model.cameras[gltfNode.camera];
			ParseCamera(camera, &(*node));
		}

		/*if (auto extension = get_extension(gltf_node.extensions, KHR_LIGHTS_PUNCTUAL_EXTENSION))
		{
			auto& lights = model.lights[gltfNode.light];
			auto light = lights.at(static_cast<size_t>(extension->Get("light").Get<int>()));

			node->set_component(*light);

			light->set_node(*node);
		}*/

		nodes.push_back(node);
	}

	// Load scenes
	std::queue<std::pair<GameObject*, int>> traverseNodes;

	tinygltf::Scene* gltf_scene{ nullptr };

	if (scene_index >= 0 && scene_index < static_cast<int>(model.scenes.size()))
	{
		gltf_scene = &model.scenes[scene_index];
	}
	else if (model.defaultScene >= 0 && model.defaultScene < static_cast<int>(model.scenes.size()))
	{
		gltf_scene = &model.scenes[model.defaultScene];
	}
	else if (model.scenes.size() > 0)
	{
		gltf_scene = &model.scenes[0];
	}

	if (!gltf_scene)
	{
		throw std::runtime_error("Couldn't determine which scene to load!");
	}

	auto rootNode = WL_NEW(GameObject)(gltf_scene->name);
	auto rootTransform = rootNode->GetComponent<Transform>();

	for (auto nodeIndex : gltf_scene->nodes)
	{
		traverseNodes.push(std::make_pair(&(*rootNode), nodeIndex));
	}

	while (!traverseNodes.empty())
	{
		auto nodeIt = traverseNodes.front();
		traverseNodes.pop();

		auto& currentNode = *nodes.at(nodeIt.second);
		auto& traverseRootNode = nodeIt.first;

	 	Transform* currentNodeTransform = currentNode.GetComponent<Transform>();
		Transform* traverseRootNodeTransform = currentNode.GetComponent<Transform>();
		currentNodeTransform->SetParent(traverseRootNodeTransform);

		for (auto childNodeIndex : model.nodes[nodeIt.second].children)
		{
			traverseNodes.push(std::make_pair(&(currentNode), childNodeIndex));
		}
	}

	scene->SetRootNode(rootNode);
	nodes.push_back(std::move(rootNode));

	// Store nodes into the scene
	scene->SetNodes(std::move(nodes));

	return scene;
}

void GltfReader::LoadLight(tinygltf::Model& model)
{
	if (IsExtensionEnabled(KHR_LIGHTS_PUNCTUAL_EXTENSION))
	{
		if (model.extensions.find(KHR_LIGHTS_PUNCTUAL_EXTENSION) == model.extensions.end() || !model.extensions.at(KHR_LIGHTS_PUNCTUAL_EXTENSION).Has("lights"))
		{
			return;
		}

		auto& khrLights = model.extensions.at(KHR_LIGHTS_PUNCTUAL_EXTENSION).Get("lights");
		for (size_t lightIndex = 0; lightIndex < khrLights.ArrayLen(); ++lightIndex)
		{
			auto& khrLight = khrLights.Get(lightIndex);
			if (!khrLights.Has("type"))
			{
				throw std::runtime_error("Couldn't load glTF file, KHR_lights_punctual extension is invalid");
			}

			GameObject* go = CreateGameObject("light");
			Light* light = go->AddComponent<Light>();
			
			LightType type;
			LightProperties properties;

			auto& gltfLightType = khrLights.Get("type").Get<std::string>();
			if (gltfLightType == "point")
			{
				type = LightType::Point;
			}
			else if (gltfLightType == "spot")
			{
				type = LightType::Spot;
			}
			else if (gltfLightType == "directional")
			{
				type = LightType::Directional;
			}
			else
			{
				throw std::runtime_error("Couldn't load glTF file, KHR_lights_punctual extension is invalid");
			}

			if (khrLight.Has("color"))
			{
				auto& color = khrLight.Get("color");
				properties.color = glm::vec3(
					static_cast<float>(color.Get(0).Get<double>()),
					static_cast<float>(color.Get(0).Get<double>()),
					static_cast<float>(color.Get(0).Get<double>()));
			}

			if (khrLight.Has("intensity"))
			{
				properties.intensity = static_cast<float>(khrLight.Get("intensity").Get<double>());
			}

			if (type != LightType::Directional)
			{
				properties.range = static_cast<float>(khrLight.Get("range").Get<double>());
				if (type != LightType::Point)
				{
					if (khrLight.Has("spot"))
					{
						throw std::runtime_error("Couldn't load glTF file, KHR_lights_punctual extension is invalid");
					}

					properties.innerConeAngle = static_cast<float>(khrLight.Get("spot").Get("innerConeAngle").Get<double>());
					if (khrLight.Get("spot").Has("outerConeAngle"))
					{
						properties.outerConeAngle = static_cast<float>(khrLight.Get("spot").Get("outerConeAngle").Get<double>());
					}
					else
					{
						properties.outerConeAngle = PI / 4.0f;
					}
				}
			}
			else if (type == LightType::Directional || type == LightType::Spot)
			{
				properties.direction = glm::vec3(0.0f, 0.0f, -1.0f);
			}

			light->SetLightType(type);
			light->SetLightProperties(properties);

		}
	}
}

void GltfReader::LoadMesh(tinygltf::Model& model)
{
	auto defaultMaterial = CreateDefaultMaterial();

	for (auto& gltfMesh : model.meshes)
	{
		Mesh* mesh = WL_NEW(Mesh);
		for (auto& gltfPrimitive : gltfMesh.primitives)
		{
			SubMesh subMesh;

			for (auto& attribute : gltfPrimitive.attributes)
			{
				std::string attributeName = attribute.first;
				std::transform(attributeName.begin(), attributeName.end(), attributeName.begin(), ::tolower);

				uint32_t accessorId = attribute.second;
				std::vector<uint8_t> vertexData = GetAttributeData(&model, attribute.second);
				if (attributeName == "position")
				{
					subMesh.vertexCount = model.accessors.at(attribute.second).count;
				}
				subMesh.vertexBuffers.insert(std::make_pair(attributeName, std::move(vertexData)));

				VertexAttribute attrib;
				attrib.format = GetAttributeFormat(&model, attribute.second);
				attrib.stride = GetAttributeStride(&model, attribute.second);
				subMesh.SetAttribute(attributeName, attrib);
			}

			if (gltfPrimitive.indices >= 0)
			{
				subMesh.vertexIndices = GetAttributeSize(&model, gltfPrimitive.indices);

				auto format = GetAttributeFormat(&model, gltfPrimitive.indices);
				auto indexData = GetAttributeData(&model, gltfPrimitive.indices);

				switch (format)
				{
				case VK_FORMAT_R8_UINT:
					// Converts uint8 data into uint16 data, still represented by a uint8 vector
					//indexData = convert_underlying_data_stride(index_data, 1, 2);
					//submesh->index_type = VK_INDEX_TYPE_UINT16;
					
					break;
				case VK_FORMAT_R16_UINT:
					subMesh.indexType = VK_INDEX_TYPE_UINT16;
					break;
				case VK_FORMAT_R32_UINT:
					subMesh.indexType = VK_INDEX_TYPE_UINT32;
					break;
				default:
					break;
				}
			}

			if (gltfPrimitive.material < 0)
			{
				subMesh.material = &(*defaultMaterial);
			}
			else
			{
				subMesh.material = &(*defaultMaterial);
			}

			mesh->AddSubmesh(subMesh);
		}
	}
}

bool GltfReader::IsExtensionEnabled(const std::string& requestedExtension)
{
	auto it = supportedExtensions.find(requestedExtension);
	if (it != supportedExtensions.end())
	{
		return it->second;
	}
	else
	{
		return false;
	}
}
