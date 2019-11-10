#include "ShaderManager.h"

#include "Utility/logger.h"
#include "utility/FileUtil.h"

#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/VkUtils/StringToVk.h"
#include "VulkanAPI/VkUtils/VkToString.h"

namespace VulkanAPI
{

ShaderManager::ShaderManager(VkContext& context)
    : context(context)
{
}

ShaderManager::~ShaderManager()
{
}

void ShaderManager::prepareBindings(ShaderCompilerInfo::ShaderDescriptor* shader, ShaderInfo& shaderInfo,
                                    uint16_t& bind)
{
	// add the glsl version number
	shader->appendBlock += "#version 450\n";

	// texture samplers
	if (!shader->samplers.empty())
	{
		for (auto& sampler : shader->samplers)
		{
			std::string inputLine;
			DescriptorLayout& layout = shaderInfo.descrLayouts[sampler.groupId];

			VkUtils::createVkShaderInput(sampler.name, sampler.type, bind, setId, inputLine);
			shader->appendBlock += inputLine + "\n";

			// store the binding data for vk descriptor creation
			layout.add(sampler.groupId, bind, vk::DescriptorType::eCombinedImageSampler,
                       Shader::getStageFlags(shader->type);
		}
	}

	// uniform buffers
	if (!shader->ubos.empty())
	{
		for (auto& buffer : shader->ubos)
		{
			std::string inputLine;
			uint32_t bufferSize;
			DescriptorLayout& layout = shaderInfo.descrLayouts[buffer.groupId];

			VkUtils::createVkShaderBuffer(buffer.name, buffer.type, buffer.data, bind, setId, inputLine, bufferSize);
			shader->appendBlock += inputLine + "\n";

			vk::DescriptorType descrType = getVkDescrTypeFromStr(buffer.type);

			// add the layout to the descriptors
			layout.add(buffer.groupId, bind, descrType,
                                       Shader::getStageFlags(shader->type);
		}
	}

	// push blocks

	// specialisation constants
}

void ShaderManager::writeInputs(ShaderCompilerInfo::ShaderDescriptor* shader,
                                ShaderCompilerInfo::ShaderDescriptor* nextShader)
{
	uint16_t loc = 0;

	for (auto& output : shader->outputs)
	{
		std::string inputLine = "layout (location = " + std::to_string(loc) + ") out " + output.type;
		std::string outputLine = "layout (location = " + std::to_string(loc) + ") in " + output.type;
		shader->appendBlock += outputLine + "\n";
		nextShader->appendBlock += inputLine + "\n";
	}
}

void ShaderManager::prepareInputs(ShaderCompilerInfo::ShaderDescriptor* vertShader, ShaderInfo& shaderInfo)
{
	if (vertShader->inputs.empty())
	{
		return;
	}

	uint16_t loc = 0;
	for (auto& input : vertShader->inputs)
	{
		std::string inputLine = "layout (location = " + std::to_string(loc) + ") in " + input.type;

		vertShader->appendBlock += inputLine + "\n";

		uint32_t stride = Shader::getStrideFromType(input.type);
		vk::Format format = Shader::getVkFormatFromType(input.type, input.width);
		shaderInfo.pipeline.addVertexInput(loc++, format, stride);
	}
}

void ShaderManager::prepareOutputs(ShaderCompilerInfo& compilerInfo, ShaderInfo& shaderInfo)
{
	ShaderCompilerInfo::ShaderDescriptor* currShader = compilerInfo.vertShader;
	// link the output semantics from one shader stage with the inputs to the next
	// shader stage
	do
	{
		if (!currShader->outputs.empty())
		{
			writeInputs(currShader, currShader->nextStage);
		}
		currShader = currShader->nextStage;
	} while (currShader->nextStage);

	// finish by writing the fragment shader render targets, if declared
	ShaderCompilerInfo::ShaderDescriptor* fragShader = compilerInfo.vertShader;
	if (fragShader && !fragShader->outputs.empty())
	{
		uint16_t loc = 0;
		for (auto& output : fragShader->outputs)
		{
			std::string inputLine = "layout (location = " + std::to_string(loc) + ") out " + output.type;
			fragShader->appendBlock += inputLine + "\n";

			vk::Format format = Shader::getVkFormatFromType(output.type, output.width);
			ShaderInfo::RenderTarget binding{ loc++, format };
			shaderInfo.renderTargets.emplace_back(binding);
		}
	}
}

bool ShaderManager::compile(ShaderCompilerInfo& compilerInfo)
{
	// sanity check first - must have a vertex shader at least
	if (!compilerInfo.vertShader)
	{
		LOGGER_ERROR("The shader doesn't contain a vertex stage. This is mandatory.");
		return false;
	}

	uint16_t bind = 0;
	ShaderInfo shaderInfo;

	// adjust the number of layouts to the max group size
	shaderInfo.descrLayouts.resize(compilerInfo.groupSize + 1);

	// prepare the bindings for each stage
	ShaderCompilerInfo::ShaderDescriptor* currShader = compilerInfo.vertShader;
	do
	{
		prepareBindings(compilerInfo.vertShader, shaderInfo, bind);
		currShader = currShader->nextStage;
	} while (currShader->nextStage);

	// prepare the input semantics, this is only required for the vertex shader
	prepareInputs(compilerInfo.vertShader, shaderInfo);

	// and link the output from each shader stage, with the input of the next
	prepareOutputs(compilerInfo, shaderInfo);

	// finalise the shder code blocks and compile into glsl byte code
	Shader shader(context);

	currShader = compilerInfo.vertShader;
	do
	{
		currShader->appendBlock += currShader->code;
		shader.add(currShader->appendBlock, currShader->type);
		currShader = currShader->nextStage;

	} while (currShader->nextStage);

	// now we have all the data required from the shader, create some of the vulkan
	// resources now to save time later
	// Create the descriptor layouts for each set
	for (const DescriptorLayout& layout : shaderInfo.descrLayouts)
	{
		layout.prepare(context);
	}

	// create the pipeline layout - as we know the descriptor layout and any push blocks
	shaderInfo.pLineLayout.prepare(context, shaderInfo.descrLayouts);

	// now save the shader based on a hash: name,
}

bool ShaderManager::readShader(rapidjson::Document& doc, ShaderCompilerInfo::ShaderDescriptor& shader, std::string id,
                               uint16_t& maxGroup)
{
	maxGroup = 0;
	const rapidjson::Value& vert = doc[id];

	// input semantics - glsl code: layout (location = 0) in [TYPE] [NAME]
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
	// output semantics - glsl code: layout (location = 0) out [TYPE] [NAME]
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

	// all samplers, ubos, constant values to import
	if (doc.HasMember("Import"))
	{
		const auto& imports = doc["Import"].GetArray();
		if (!imports.Empty())
		{
			for (auto& import : imports)
			{
				std::string name = import["name"].GetString();
				std::string type = import["type"].GetString();

				// samplers and buffers can be grouped into sets - this is optional and if
				// not specified, all imports will be grouped in the same set
				uint16_t group = 0;
				if (import.HasMember("group"))
				{
					group = std::to_string(import["group"].GetString()) maxGroup = group;
				}

				if (VkUtils::isSamplerType(name))
				{
					shader.samplers.emplace_back(name, type group);
				}
				else if (VkUtils::isBufferType(name))
				{
					// extra data for a buffer: the types and names of the buffer
					ShaderCompilerInfo::ShaderDescriptor::BufferDescriptor buffer;
					buffer.descr{ name, type, goup };

					const auto& items = import["Items"].GetArray();

					for (auto& item : items)
					{
						std::string itemName = items["name"].GetString();
						std::string itemType = items["type"].GetString();

						buffer.descr.data.emplace_back(itemName, itemType);
					}
					shader.ubos.emplace_back(buffer);
				}
			}
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

bool ShaderManager::parseShaderJson(std::string inputJson, ShaderCompilerInfo& compilerInfo)
{
	rapidjson::Document document;

	if (document.Parse(inputJson.c_str()).HasParseError())
	{
		LOGGER_ERROR("Unable to load shader file: %s.", filename.c_str());
		return false;
	}

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

	if (doc.HasMember("ComputeShader"))
	{
		compilerInfo.compShader = std::make_unique<ShaderCompilerInfo::Shader>();
		if (!readShader(doc, *compilerInfo.compShader, "ComputeShader"))
		{
			return false;
		}
	}
	else
	{
		// used for creating a linked list of shaders - which will be required for
		// creating the input/output semantics of each stage
		ShaderCompilerInfo::Shader* prevStage = nullptr;

		// definition must have at least a vertex shader stage
		// all other stages are optional
		compilerInfo.vertShader = std::make_unique<ShaderCompilerInfo::Shader>();
		if (!readShader(doc, *compilerInfo.vertShader, "VertexShader", compilerInfo.groupSize))
		{
			return false;
		}
		prevStage = compilerInfo.vertShader.get();

		// this encompasses the control and evaluation stages
		if (doc.HasMember("TesselationShader"))
		{
			compilerInfo.tessShader = std::make_unique<ShaderCompilerInfo::Shader>();
			if (!readShader(doc, *compilerInfo.tessShader, "TesselationShader", compilerInfo.groupSize))
			{
				return false;
			}
			compilerInfo.prevShader->nextStage = compilerInfo.tessShader.get();
			prevStage = compilerInfo.tessShader.get();
		}

		if (doc.HasMember("GeometryShader"))
		{
			compilerInfo.geomShader = std::make_unique<ShaderCompilerInfo::Shader>();
			if (!readShader(doc, *compilerInfo.geomShader, "GeometryShader", compilerInfo.groupSize))
			{
				return false;
			}
			compilerInfo.prevShader->nextStage = compilerInfo.geomShader.get();
			prevStage = compilerInfo.geomShader.get();
		}

		if (doc.HasMember("FragmentShader"))
		{
			compilerInfo.fragShader = std::make_unique<ShaderCompilerInfo::Shader>();
			if (!readShader(doc, *compilerInfo.fragShader, "FragmentShader", compilerInfo.groupSize))
			{
				return false;
			}
			compilerInfo.prevShader->nextStage = compilerInfo.fragShader.get();
			prevStage = compilerInfo.fragShader.get();
		}
	}

	return true;
}

bool ShaderManager::load(Util::String filename, std::string output)
{
	if (!FileUtil::readFileIntoBuffer(filename.c_str(), output))
	{
		return false;
	}

	return true;
}

bool ShaderManager::prepare(std::string inputJson)
{
	ShaderCompilerInfo compilerInfo;
	if (!parseShaderJson(inputJson, compilerInfo))
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

void ShaderManager::addVariation(Util::CString definition, uint8_t value)
{
}

}    // namespace VulkanAPI
