
#pragma once

#include <unordered_map>

#include <volk.h>

class Material;

struct VertexAttribute
{
	VkFormat format = VK_FORMAT_UNDEFINED;

	uint32_t stride = 0;
	uint32_t offset = 0;
};

struct SubMesh
{
	uint32_t vertexCount;
	uint32_t vertexIndices;
	VkIndexType indexType;
	const Material* material{ nullptr };
	std::unordered_map<std::string, std::vector<uint8_t>> vertexBuffers;

	inline void SetAttribute(const std::string& name, const VertexAttribute& attribute)
	{
		vertexAttributes[name] = attribute;
	}

private:
	std::unordered_map<std::string, VertexAttribute> vertexAttributes;
};

class Mesh
{
public:
	Mesh();
	//~Mesh();

	inline void AddSubmesh(SubMesh& submesh)
	{
		submeshes.push_back(submesh);
	}
private:

	std::vector<SubMesh> submeshes;
};
