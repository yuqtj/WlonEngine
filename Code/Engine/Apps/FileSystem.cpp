
#include "FileSystem.h"
#include <fstream>

FileSystem FileSystem::s_Instance;

FileSystem& FileSystem::GetInstance()
{
	return s_Instance;
}

void FileSystem::Initialized()
{

}

std::vector<uint8_t> FileSystem::LoadFile(const std::string& filename)
{
	std::vector<uint8_t> data;
	std::ifstream file;

	file.open(filename, std::ios::in | std::ios::binary);
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file: " + filename);
	}

	const uint32_t count = 0;
	uint64_t readCount = count;
	if (count == 0)
	{
		file.seekg(0, std::ios::end);
		readCount = static_cast<uint64_t>(file.tellg());
		file.seekg(0, std::ios::beg);
	}

	data.resize(static_cast<size_t>(readCount));
	file.read(reinterpret_cast<char*>(data.data()), readCount);
	file.close();

	return data;
}
