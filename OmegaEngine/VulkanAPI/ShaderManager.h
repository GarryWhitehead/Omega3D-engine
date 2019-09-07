#pragma once
#include <shaderc/shaderc.hpp>

#include <string>
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

private:
	std::vector<uint32_t> output;

	shaderc_shader_kind kind;
	std::string source;
	std::string sourceName;
	std::unordered_map<std::string, uint8_t> defines;
};

class ShaderManager
{
public:
	ShaderManager();
	~ShaderManager();

private:
};

}    // namespace VulkanAPI
