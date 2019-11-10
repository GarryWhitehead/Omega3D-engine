#pragma once
#include <shaderc/shaderc.hpp>

#include "VulkanAPI/Common.h"

#include "utility/String.h"

#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

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

	uint32_t* getData()
	{
		return output.data();
	}

	size_t getSize() const
	{
		return output.size();
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
	struct ShaderCompilerInfo
	{
		struct DepthStencilState
		{
			bool testEnable = false;
			bool writeEnable = false;
			vk::CompareOp compareOp = vk::CompareOp::eLessOrEqual;
		};

		struct RasterState
		{
			vk::CullModeFlagBits cullMode = vk::CullModeFlagBits::eNone;
			vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
			vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
		};

		struct Sampler
		{
			vk::Filter magFilter;
			vk::Filter minFilter;
			vk::SamplerAddressMode addrModeU;
			vk::SamplerAddressMode addrModeV;
			vk::SamplerAddressMode addrModeW;
		};

		struct Shader
		{
			std::unordered_map<std::string, std::string> inputs;
			std::unordered_map<std::string, std::string> outputs;
			std::vector<std::string> code;
		};

		DepthStencilState dsState;
		RasterState rasterState;
		Sampler sampler;

		std::unique_ptr<Shader> vertShader;
		std::unique_ptr<Shader> fragShader;

		std::unordered_map<std::string, std::string> imports;
	};

	ShaderManager();
	~ShaderManager();

	bool readShader(rapidjson::Document& doc, ShaderCompilerInfo::Shader& shader, std::string id);

	bool load(Util::String filename);

	bool parseShaderJson(rapidjson::Document& doc, ShaderCompilerInfo& compilerInfo);

	bool compile(ShaderCompilerInfo& info);

private:
};

}    // namespace VulkanAPI