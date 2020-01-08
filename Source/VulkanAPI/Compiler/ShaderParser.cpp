#include "ShaderParser.h"

#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/VkUtils/StringToVk.h"
#include "VulkanAPI/VkUtils/VkToString.h"

#include "utility/FileUtil.h"
#include "utility/Logger.h"

namespace VulkanAPI
{

// ================== ShaderParser =======================
bool ShaderParser::readShader(rapidjson::Document& doc, ShaderDescriptor& shader, Util::String id, uint16_t& maxGroup)
{
	maxGroup = 0;
	const auto& shaderBlock = doc[id.c_str()];

	// input semantics - glsl code: layout (location = 0) in [TYPE] [NAME]
	if (shaderBlock.HasMember("Inputs"))
	{
		const auto& inputs = shaderBlock["Inputs"].GetArray();
		for (auto& input : inputs)
		{
			std::string name = input["name"].GetString();
			std::string type = input["type"].GetString();
            shader.inputs.emplace_back(ShaderDescriptor::InOutDescriptor{ name, type });
		}
	}
	// output semantics - glsl code: layout (location = 0) out [TYPE] [NAME]
	if (shaderBlock.HasMember("Outputs"))
	{
		const auto& outputs = shaderBlock["Outputs"].GetArray();
		for (auto& output : outputs)
		{
			std::string name = output["name"].GetString();
			std::string type = output["type"].GetString();
            shader.outputs.emplace_back(ShaderDescriptor::InOutDescriptor{ name, type });
		}
	}

	// speciliastion constants - should be preferred to the usual #define method
	if (shaderBlock.HasMember("Constants"))
	{
		const auto& constants = shaderBlock["Constants"].GetArray();
		for (auto& constant : constants)
		{
			ShaderDescriptor::ConstantDescriptor descr;
			descr.name = constant["name"].GetString();
			descr.type = constant["type"].GetString();
			descr.value = constant["value"].GetString();
			shader.constants.emplace_back(descr);
		}
	}

	// push constants - one per shader stage supported - due to the size limit that can be pushed,
	// this shouldn't be an issue
	if (shaderBlock.HasMember("PushConstants"))
	{
		const auto& constants = shaderBlock["PushConstants"].GetArray();
		for (auto& constant : constants)
		{
			ShaderDescriptor::PConstantDescriptor descr;
			descr.name = constant["name"].GetString();
			descr.type = constant["type"].GetString();

			if (constant.HasMember("id"))
			{
				descr.id = constant["id"].GetString();
			}

			const auto& items = constant["items"].GetArray();
			for (auto& item : items)
			{
				ShaderDescriptor::Descriptor itemDescr;
				itemDescr.name = item["name"].GetString();
				itemDescr.type = item["type"].GetString();

				descr.data.emplace_back(itemDescr);
			}
			shader.pConstants.emplace_back(descr);
		}
	}

	// all samplers, ubos to import
	if (shaderBlock.HasMember("Import"))
	{
		const auto& imports = shaderBlock["Import"].GetArray();
		if (!imports.Empty())
		{
			for (auto& import : imports)
			{
				std::string name = import["name"].GetString();
				std::string type = import["type"].GetString();

				// samplers and buffers can be grouped using a identifying name
                // The id will be appended to the buffer or sampler name
				uint16_t group = 0;
				if (import.HasMember("group"))
				{
                    group = std::stoi(import["group"].GetString());
				}

				if (VkUtils::isSamplerType(name))
				{
                    ShaderDescriptor::Descriptor samplerDescr;
                    samplerDescr.name = name;
                    samplerDescr.type = type;
                    samplerDescr.groupId = group;
                    
                    std::string variant;
					if (import.HasMember("variant"))
					{
						samplerDescr.variant = import["variant"].GetString();
					}
         
                    shader.samplers.emplace_back(samplerDescr);
				}
				else if (VkUtils::isBufferType(name))
				{
					// extra data for a buffer: the types and names of the buffer
					ShaderDescriptor::BufferDescriptor buffer;
					buffer.descr.name = name;
					buffer.descr.type = type;
					buffer.descr.groupId = group;

					// optional sub id for a uniform buffer - e.g.
					//	struct Blah
					// {
					// } sub_name;
					if (import.HasMember("id"))
					{
						buffer.descr.id = import["id"].GetString();
					}

					const auto& items = import["items"].GetArray();
					for (auto& item : items)
					{
						ShaderDescriptor::Descriptor itemDescr;
						itemDescr.name = item["name"].GetString();
						itemDescr.type = item["type"].GetString();

						// check whether any arrays are specified
						if (item.HasMember("array_constant"))
						{
							itemDescr.arrayConst = item["array_constant"].GetString();
						}
						if (item.HasMember("array_value"))
						{
							itemDescr.arraySize = item["array_value"].GetInt();
						}
						buffer.data.emplace_back(itemDescr);
					}
					shader.ubos.emplace_back(buffer);
				}
			}
		}
	}

	// a code block is not optional
	if (!shaderBlock.HasMember("Code"))
	{
		LOGGER_ERROR("No code block found for %s.", id.c_str());
		return false;
	}

	const auto& codeBlock = shaderBlock["Code"].GetArray();
	for (auto& c : codeBlock)
	{
		shader.code += c.GetString();
	}
    
    return true;
}

bool ShaderParser::prepareShader(Util::String filename, ShaderDescriptor* shader, Shader::Type type)
{
	std::string shaderBuffer;
	if (!FileUtil::readFileIntoBuffer(filename.c_str(), buffer))
	{
		return false;
	}

	rapidjson::Document doc;

	if (doc.Parse(buffer.c_str()).HasParseError())
	{
		LOGGER_ERROR("Error whilst trying to parse shader json file.");
		return false;
	}

	Util::String vertexId = Shader::shaderTypeToString(type);
	uint16_t maxGroup = 0;

	if (!readShader(doc, *shader, vertexId, maxGroup))
	{
		LOGGER_ERROR("Unable to read shader block from json file; filename = %s.", filename.c_str());
		return false;
	}
    
    return true;
}

void ShaderParser::addStage(ShaderDescriptor* shader)
{
	descriptors.emplace_back(shader);
}

void ShaderParser::parseRenderBlock(rapidjson::Document& doc)
{
	// parse all the raster state data from the file :
	// Note: rather than storing this data in a raw string fromat and then dealing with it later in the compiler,
	// the render state data is extracted in it's final format and passed to the compiler later
	if (doc.HasMember("DepthStencilState"))
	{
		const rapidjson::Value& depth = doc["DepthState"];
		if (depth.HasMember("DepthTestEnable"))
		{
            renderState->dsState.testEnable = depth["DepthTestEnable"].GetBool();
		}
		if (depth.HasMember("DepthWriteEnable"))
		{
            renderState->dsState.writeEnable = depth["DepthWriteEnable"].GetBool();
		}
		if (depth.HasMember("CompareOp"))
		{
            renderState->dsState.compareOp = VkUtils::vkCompareOpFromString(depth["CompareOp"].GetString());
		}
	}

	if (doc.HasMember("RasterState"))
	{
		const rapidjson::Value& rState = doc["RasterState"];
		if (rState.HasMember("PolygonMode"))
		{
			renderState->rastState.polygonMode = VkUtils::vkPolygonFromString(rState["PolygonMode"].GetString());
		}
		if (rState.HasMember("CullMode"))
		{
			renderState->rastState.cullMode = VkUtils::vkCullModeFromString(rState["CullMode"].GetString());
		}
		if (rState.HasMember("FrontFace"))
		{
			renderState->rastState.frontFace = VkUtils::vkFrontFaceFromString(rState["FrontFace"].GetString());
		}
	}

	if (doc.HasMember("Sampler"))
	{
		const rapidjson::Value& sampler = doc["Sampler"];
		if (sampler.HasMember("MagFilter"))
		{
            renderState->sampler.magFilter = VkUtils::vkFilterToString(sampler["MagFilter"].GetString());
		}
		if (sampler.HasMember("MinFilter"))
		{
			renderState->sampler.minFilter = VkUtils::vkFilterToString(sampler["MinFilter"].GetString());
		}
		if (sampler.HasMember("AddressModeU"))
		{
			renderState->sampler.addrModeU = VkUtils::vkAddressModeToString(sampler["AddressModeU"].GetString());
		}
		if (sampler.HasMember("AddressModeV"))
		{
			renderState->sampler.addrModeV = VkUtils::vkAddressModeToString(sampler["AddressModeV"].GetString());
		}
		if (sampler.HasMember("AddressModeW"))
		{
			renderState->sampler.addrModeW = VkUtils::vkAddressModeToString(sampler["AddressModeW"].GetString());
		}
	}
}

bool ShaderParser::parseShaderJson()
{
	rapidjson::Document doc;

	if (doc.Parse(buffer.c_str()).HasParseError())
	{
		LOGGER_ERROR("Error whilst trying to parse shader json file.");
		return false;
	}

	parseRenderBlock(doc);

	// If a compute shdaer life is a bit easier, otherwise check for each shader stage and extract all information into a raw
	// format for the compiler to use
	if (doc.HasMember("ComputeShader"))
	{
		auto compShader = std::make_unique<ShaderDescriptor>();
		if (!readShader(doc, *compShader, "ComputeShader", groupSize))
		{
			return false;
		}
		descriptors.emplace_back(std::move(compShader));
	}
	else
	{
		if (doc.HasMember("VertexShader"))
		{
			auto vertShader = std::make_unique<ShaderDescriptor>();
			if (!readShader(doc, *vertShader, "VertexShader", groupSize))
			{
				return false;
			}
			descriptors.emplace_back(std::move(vertShader));
		}

		// this encompasses the control and evaluation stages
		if (doc.HasMember("TesselationShader"))
		{
			auto tessEvalShader = std::make_unique<ShaderDescriptor>();
			if (!readShader(doc, *tessEvalShader, "TesselationShader", groupSize))
			{
				return false;
			}
			descriptors.emplace_back(std::move(tessEvalShader));
		}

		if (doc.HasMember("GeometryShader"))
		{
			auto geomShader = std::make_unique<ShaderDescriptor>();
			if (!readShader(doc, *geomShader, "GeometryShader", groupSize))
			{
				return false;
			}
			descriptors.emplace_back(std::move(geomShader));
		}

		if (doc.HasMember("FragmentShader"))
		{
			auto fragShader = std::make_unique<ShaderDescriptor>();
			if (!readShader(doc, *fragShader, "FragmentShader", groupSize))
			{
				return false;
			}
			descriptors.emplace_back(std::move(fragShader));
		}
	}

	return true;
}

bool ShaderParser::parse(Util::String filename)
{
	if (!FileUtil::readFileIntoBuffer(filename.c_str(), buffer))
	{
		return false;
	}

	if (!parseShaderJson())
	{
		return false;
	}

	return true;
}
}    // namespace VulkanAPI