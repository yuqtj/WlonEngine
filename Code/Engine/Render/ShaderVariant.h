#pragma once

#include <string>
#include <vector>
#include <unordered_map>

class ShaderVariant
{
public:
	ShaderVariant() = default;
	ShaderVariant(std::string&& preamble, std::vector<std::string>&& processes);

	size_t GetId() const;
	const std::string& GetPreamble() const;
	const std::vector<std::string>& GetProcesses() const;
protected:
private:
	size_t id;
	std::string preamble;
	std::vector<std::string> processes;
	std::unordered_map<std::string, size_t> runtimeArraySizes;

	void UpdateID();
};