#pragma once

#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
#include <volk.h>

#include "Render/ShaderVariant.h"

class GlslCompiler
{
public:
	static bool CompilerToSpriv(VkShaderStageFlagBits stage, const std::vector<uint8_t>& glslSource, const std::string& entryPoint,
		const ShaderVariant& shaderVariant, std::vector<std::uint32_t>& spirv, std::string& infoLog);
private:
	GlslCompiler() {};
	~GlslCompiler() {};
};
