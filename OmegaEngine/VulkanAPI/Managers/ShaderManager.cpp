#include "ShaderManager.h"

#include "Utility/logger.h"
#include "utility/FileUtil.h"

#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkUtils/StringToVk.h"

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
	bool success = FileUtil::readFileIntoBuffer(filename, this->source);

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

// =============================== ShaderManager ===========================================

ShaderManager::ShaderManager()
{
}

ShaderManager::~ShaderManager()
{
}

bool ShaderManager::compile(ShaderCompilerInfo& compilerInfo)
{
	// first we need to append the input data to each shader text files
	if (!compilerInfo.vertShader)
	{
		LOGGER_ERROR("The shader doesn't contain a vertex stage. This mandatory.");
		return false;
	}

	std::vector<std::string> inputBlock;
	if (!compilerInfo.vertShader->inputs.empty())
	{
		size_t setCount = 0;
		for (uint16_t i = 0, bind = 0; i < compilerInfo.vertShader->inputs.size(); ++i, ++bind)
		{
			// first = name; second = type
			std::string inputLine;
			createVkShaderInput(input.first, input.second, bind, setCount, inputLine);
		}
	}
}

bool ShaderManager::readShader(rapidjson::Document& doc, ShaderCompilerInfo::Shader& shader, std::string id)
{
	const rapidjson::Value& vert = doc["VertexShader"];
	if (vert.HasMember("Inputs"))
	{
		const auto& inputs = vert["Inputs"].GetArray();
		for (auto& input : inputs)
		{
			std::string name = input["name"].GetString();
			std::string type = input["type"].GetString();
			shader.inputs.emplace(name, type);
		}
	}
	if (vert.HasMember("Outputs"))
	{
		const auto& outputs = vert["Outputs"].GetArray();
		for (auto& output : outputs)
		{
			std::string name = output["name"].GetString();
			std::string type = output["type"].GetString();
			shader.outputs.emplace(name, type);
		}
	}

	// a code block is not optional
	if (!vert.HasMember("Code"))
	{
		LOGGER_ERROR("No code block found for %s.", id.c_str());
		return false;
	}

	const auto& codeBlock = vert["Code"].GetArray();
	for (auto& c : codeBlock)
	{
		shader.code.push_back(c.GetString());
	}
}

bool ShaderManager::parseShaderJson(rapidjson::Document& doc, ShaderCompilerInfo& compilerInfo)
{
	if (doc.HasMember("DepthStencilState"))
	{
		const rapidjson::Value& dsState = doc["DepthStencilState"];
		if (dsState.HasMember("DepthTestEnable"))
		{
			compilerInfo.dsState.testEnable = dsState["DepthTestEnable"].GetBool();
		}
		if (dsState.HasMember("DepthWriteEnable"))
		{
			compilerInfo.dsState.writeEnable = dsState["DepthWriteEnable"].GetBool();
		}
		if (dsState.HasMember("CompareOp"))
		{
			compilerInfo.dsState.compareOp = VkUtils::vkCompareOpFromString(dsState["CompareOp"].GetString());
		}
	}

	if (doc.HasMember("RasterState"))
	{
		const rapidjson::Value& rState = doc["RasterState"];
		if (rState.HasMember("PolygonMode"))
		{
			compilerInfo.rasterState.polygonMode = VkUtils::vkPolygonFromString(rState["PolygonMode"].GetString());
		}
		if (rState.HasMember("CullMode"))
		{
			compilerInfo.rasterState.cullMode = VkUtils::vkCullModeFromString(rState["CullMode"].GetString());
		}
		if (rState.HasMember("FrontFace"))
		{
			compilerInfo.rasterState.frontFace = VkUtils::vkFrontFaceFromString(rState["FrontFace"].GetString());
		}
	}

	if (doc.HasMember("Sampler"))
	{
		const rapidjson::Value& sampler = doc["Sampler"];
		if (sampler.HasMember("MagFilter"))
		{
			compilerInfo.sampler.magFilter = VkUtils::vkFilterToString(sampler["MagFilter"].GetString());
		}
		if (sampler.HasMember("MinFilter"))
		{
			compilerInfo.sampler.minFilter = VkUtils::vkFilterToString(sampler["MinFilter"].GetString());
		}
		if (sampler.HasMember("AddressModeU"))
		{
			compilerInfo.sampler.addrModeU = VkUtils::vkAddressModeToString(sampler["AddressModeU"].GetString());
		}
		if (sampler.HasMember("AddressModeV"))
		{
			compilerInfo.sampler.addrModeV = VkUtils::vkAddressModeToString(sampler["AddressModeV"].GetString());
		}
		if (sampler.HasMember("AddressModeW"))
		{
			compilerInfo.sampler.addrModeW = VkUtils::vkAddressModeToString(sampler["AddressModeW"].GetString());
		}
	}

	if (doc.HasMember("Import"))
	{
		const rapidjson::Value& imports = doc["Import"].GetArray();
		if (!imports.Empty())
		{
			for (auto& import : imports)
			{
				std::string name = import["name"].GetString();
				std::string type = import["type"].GetString();
				compilerInfo.imports.emplace(name, type);
			}
		}
	}

	if (doc.HasMember("VertexShader"))
	{
		compilerInfo.vertShader = std::make_unique<ShaderCompilerInfo::Shader>();
		if (!readShader(doc, *compilerInfo.vertShader, "VertexShader"))
		{
			return false;
		}
	}

	if (doc.HasMember("FragmentShader"))
	{
		compilerInfo.fragShader = std::make_unique<ShaderCompilerInfo::Shader>();
		if (!readShader(doc, *compilerInfo.fragShader, "FragmentShader"))
		{
			return false;
		}
	}
}

bool ShaderManager::load(Util::String filename)
{
	rapidjson::Document document;
	std::string json;

	if (!FileUtil::readFileIntoBuffer(filename.c_str(), json))
	{
		return false;
	}

	if (document.Parse(json.c_str()).HasParseError())
	{
		LOGGER_ERROR("Unable to load shader file: %s.", filename.c_str());
		return false;
	}

	// first extract all the raw string data from the json file
	ShaderCompilerInfo compilerInfo;
	if (!parseShaderJson(document, compilerInfo))
	{
		return false;
	}

	// now compile into shader bytecode and the assoicated rendering state data
	if (!compile(compilerInfo))
	{
		return false;
	}

	return true;
}

}    // namespace VulkanAPI
