
#include "Render/ShaderVariant.h"

ShaderVariant::ShaderVariant(std::string&& preamble, std::vector<std::string>&& processes) : 
	preamble{ std::move(preamble) },
	processes{ std::move(processes) }
{
	UpdateID();
}

size_t ShaderVariant::GetId() const
{
	return id;
}

const std::string& ShaderVariant::GetPreamble() const
{
	return preamble;
}

const std::vector<std::string>& ShaderVariant::GetProcesses() const
{
	return processes;
}

void ShaderVariant::UpdateID()
{
	std::hash<std::string> hasher{};
	id = hasher(preamble);
}

