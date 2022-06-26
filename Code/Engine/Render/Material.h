
#pragma once

#include <string>
#include <unordered_map>

#include "Framework/GlmCommon.h"

enum class AlphaMode
{
	/// Alpha value is ignored
	Opaque,
	/// Either full opaque or fully transparent
	Mask,
	/// Output is combined with the background
	Blend
};

class Texture;

class Material
{
public:
	Material(std::string name)
	{
	}

	Material(Material&& other) = default;

	glm::vec4 baseColorFactor{ 0.0f, 0.0f, 0.0f, 0.0f };

	float metallicFactor{ 0.0f };

	float roughnessFactor{ 0.0f };

	std::unordered_map<std::string, Texture*> textures;
	glm::vec3 emissive{ 0.0f, 0.0f, 0.0f };
	bool doubleSided{ false };
	float alphaCutoff{ 0.5f };
	/// Alpha rendering mode
	AlphaMode alphaMode{ AlphaMode::Opaque };
};