#pragma once

#include <string>

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

class Scene;

class GltfReader
{
public:
	GltfReader() {};
	~GltfReader() {};

	static Scene* LoadFile(const char* path);

private:
	static void LoadLight(tinygltf::Model& model);
	static void LoadMesh(tinygltf::Model& model);
	static bool IsExtensionEnabled(const std::string& requestedExtension);
};

