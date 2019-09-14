#include "ShaderManager.h"

#include "Utility/logger.h"
#include "utility/FileUtil.h"

#include "VulkanAPI/Shader.h"


namespace VulkanAPI
{

shaderc_shader_kind getShaderKind(const StageType type)
{
	shaderc_shader_kind result;
	switch (type)
	{
	case StageType::Vertex:
		result = shaderc_shader_kind::shaderc_vertex_shader;
		break;
	case StageType::Fragment:
		result = shaderc_shader_kind::shaderc_fragment_shader;
		break;
	case StageType::Geometry:
		result = shaderc_shader_kind::shaderc_geometry_shader;
		break;
	case StageType::Compute:
		result = shaderc_shader_kind::shaderc_compute_shader;
		break;
	}

	return result;
}

GlslCompiler::GlslCompiler(std::string filename, const StageType type)
{
	bool success = OmegaEngine::FileUtil::readFileIntoBuffer(filename, this->source);

	this->kind = getShaderKind(type);
}

GlslCompiler::~GlslCompiler()
{
}

bool GlslCompiler::compile(bool optimise)
{
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	for (auto define : defines)
	{
		options.AddMacroDefinition(define.first.c_str(), std::to_string(define.second).c_str());
	}

	if (optimise)
	{
		options.SetOptimizationLevel(shaderc_optimization_level_size);
	}

	auto result = compiler.CompileGlslToSpv(source, kind, sourceName.c_str(), options);
	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		LOGGER_INFO("%s", result.GetErrorMessage().c_str());
		return false;
	}

	std::copy(result.cbegin(), result.cend(), output.begin());

	return true;
}

ShaderManager::ShaderManager()
{
}

ShaderManager::~ShaderManager()
{
}


}    // namespace VulkanAPI