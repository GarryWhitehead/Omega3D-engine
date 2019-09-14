#pragma once
#include <shaderc/shaderc.hpp>

#include "utility/String.h"

#include <unordered_map>

namespace VulkanAPI
{

enum class StageType;

class GlslCompiler
{
public:
	GlslCompiler(std::string filename, const StageType type);
	~GlslCompiler();

	bool compile(bool optimise);

	void addDefinition(Util::String define, uint8_t value)
	{
		defines.insert(define, value);
	}
	
private:
	std::vector<uint32_t> output;

	shaderc_shader_kind kind;
	std::string source;
	std::string sourceName;
	std::unordered_map<Util::String, uint8_t> defines;
};

class ShaderManager
{
public:
	ShaderManager();
	~ShaderManager();

private:
};

}    // namespace VulkanAPI