#pragma once

#include <vector>
#include <string>

class FileSystem
{
public:
	static FileSystem& GetInstance();
	static void Initialized();
	static std::vector<uint8_t> LoadFile(const std::string& filename);
protected:
private:
	FileSystem() {};

	static FileSystem s_Instance;
};