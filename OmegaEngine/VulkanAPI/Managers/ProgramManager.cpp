#include "ProgramManager.h"

#include "Utility/logger.h"
#include "utility/FileUtil.h"

#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/VkUtils/StringToVk.h"
#include "VulkanAPI/VkUtils/VkToString.h"

namespace VulkanAPI
{

// ================== ShaderParser =======================
bool ShaderParser::readShader(rapidjson::Document& doc, ShaderDescriptor& shader, std::string id, uint16_t& maxGroup)
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
			shader.inputs.emplace(name, type);
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
			shader.outputs.emplace(name, type);
		}
	}

	// speciliastion constants - should be preferred to the usual #ddefine method
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
			shader.constants.emplace_back(descr);
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

				// samplers and buffers can be grouped into sets - this is optional and if
				// not specified, all imports will be grouped in the same set
				uint16_t group = 0;
				if (import.HasMember("group"))
				{
					group = std::stoi(import["group"].GetString());
					maxGroup = group;
				}

				if (VkUtils::isSamplerType(name))
				{
					std::string variant;
					if (import.HasMember("variant"))
					{
						variant = import["variant"].GetString();
					}
					shader.samplers.emplace_back(name, type, group, variant);
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
					if (import.HasMember["id"])
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
						if (item.HasMember["array_constant"])
						{
							itemDescr.arrayConst = item["array_constant"].GetString();
						}
						if (item.HasMember["array_value"])
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
		shader.code.push_back(c.GetString());
	}
}

bool ShaderParser::prepareShader(Util::String filename, ShaderDescriptor* shader, Shader::StageType type)
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

	Util::String vertexId = Shader::ShaderTypeToString(type);
	uint16_t maxGroup = 0;

	if (!readShader(doc, *shader, id, maxGroup))
	{
		LOGGER_ERROR("Unable to read shader block from json file; filename = %s.", filename);
		return false;
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

	// If a compute shdaer life is a bit easier, otherwise check for each shader stage and extract all information into a raw
	// format for the compiler to use
	if (doc.HasMember("ComputeShader"))
	{
		compShader = std::make_unique<ShaderCompilerInfo::Shader>();
		if (!readShader(doc, *compilerInfo.compShader, "ComputeShader"))
		{
			return false;
		}
	}
	else
	{
		// used for creating a linked list of shaders - which will be required for
		// creating the input/output semantics of each stage
		ShaderDescriptor* prevStage = nullptr;

		// definition must have at least a vertex shader stage - all other stages are optional
		vertShader = std::make_unique<ShaderDescriptor>();
		if (!readShader(doc, *vertShader, "VertexShader", groupSize))
		{
			return false;
		}
		prevStage = vertShader.get();

		// this encompasses the control and evaluation stages
		if (doc.HasMember("TesselationShader"))
		{
			tessEvalShader = std::make_unique<ShaderDescriptor>();
			if (!readShader(doc, *tessEvalShader, "TesselationShader", groupSize))
			{
				return false;
			}
			prevStage->nextStage = tessEvalShader.get();
			prevStage = tessEvalShader.get();
		}

		if (doc.HasMember("GeometryShader"))
		{
			geomShader = std::make_unique<ShaderDescriptor>();
			if (!readShader(doc, *geomShader, "GeometryShader", groupSize))
			{
				return false;
			}
			prevStage->nextStage = geomShader.get();
			prevStage = geomShader.get();
		}

		if (doc.HasMember("FragmentShader"))
		{
			fragShader = std::make_unique<ShaderDescriptor>();
			if (!readShader(doc, *fragShader, "FragmentShader", groupSize))
			{
				return false;
			}
			prevStage->nextStage = fragShader.get();
			prevStage = fragShader.get();
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

// =================== ShaderCompiler ===================
ShaderCompiler::ShaderCompiler(ShaderProgram& program)
    : program(program)
{
}

ShaderCompiler::~ShaderCompiler()
{
}

void ShaderCompiler::prepareBindings(ShaderParser::ShaderDescriptor* shader, uint16_t& bind)
{
	// add the glsl version number
	shader->appendBlock += "#version 450\n";

	// include files
	if (!shader->includeFiles.empty())
	{
		for (const auto& file : includeFiles)
		{
			// Note: I think the glsl compiler might need the absolute path - need to check
			shader->appendBlock += "#include " + file + "\n";
		}
	}

	// texture samplers
	if (!shader->samplers.empty())
	{
		for (auto& sampler : shader->samplers)
		{
			std::string inputLine;
			DescriptorLayout& layout = program.descrLayouts[sampler.groupId];

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
			DescriptorLayout& layout = program.descrLayouts[buffer.groupId];

			VkUtils::createVkShaderBuffer(buffer.name, buffer.type, buffer.data, bind, setId, inputLine, bufferSize);
			shader->appendBlock += inputLine + "\n";

			vk::DescriptorType descrType = getVkDescrTypeFromStr(buffer.type);

			// add the layout to the descriptors
			layout.add(buffer.groupId, bind, descrType,
                                       Shader::getStageFlags(shader->type);
		}
	}

	// push blocks
	if (!shader->pConstants.empty())
	{
		for (const auto& constant : shader->pConstants)
		{
			std::string inputLine;
			uint32_t bufferSize;

			// inject pipeline text into temp string block
			VkUtils::createVkShaderBuffer(constant.name, constant.type, constant.data, 0, 0, inputLine, bufferSize);
			// append to main shader text
			shader->appendBlock += inputLine + "\n";
			program.pLineLayout.addPushConstant(shader->type, bufferSize);
		}
	}

	// specialisation constants
	if (!shader->constants.empty())
	{
		uint16_t constantId = 0;
		for (const auto& constant : shader->constants)
		{
			// inject constant text into temp shader text block
			shader->appendBlock += "layout (constant_id = " + constantId + ") const " + constType + " " +
			                       constant.name + " = " + constant.value + "\n";

			program.constants.emplace_back(constant.name, constantId);
		}
	}
}

void ShaderCompiler::writeInputs(ShaderDescriptor* shader, ShaderDescriptor* nextShader)
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

void ShaderCompiler::prepareInputs(ShaderDescriptor* vertShader)
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
	}
}

void ShaderCompiler::prepareOutputs(ShaderParser& compilerInfo)
{
	auto& currShader = compilerInfo.vertShader;
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
	auto& fragShader = compilerInfo.vertShader;
	if (fragShader && !fragShader->outputs.empty())
	{
		uint16_t loc = 0;
		for (auto& output : fragShader->outputs)
		{
			std::string inputLine = "layout (location = " + std::to_string(loc) + ") out " + output.type;
			fragShader->appendBlock += inputLine + "\n";

			vk::Format format = Shader::getVkFormatFromType(output.type, output.width);
			ShaderProgram::RenderTarget binding{ loc++, format };
			program.renderTargets.emplace_back(binding);
		}
	}
}

bool ShaderCompiler::compile(ShaderParser& compilerInfo)
{
	// sanity check first - must have a vertex shader at least
	if (!compilerInfo.vertShader)
	{
		LOGGER_ERROR("The shader doesn't contain a vertex stage. This is mandatory.");
		return false;
	}

	uint16_t bind = 0;

	// adjust the number of layouts to the max group size
	program.descrLayouts.resize(compilerInfo.groupSize + 1);

	// prepare the bindings for each stage
	ShaderDescriptor* currShader = compilerInfo.vertShader.get();
	do
	{
		prepareBindings(compilerInfo.vertShader.get(), bind);
		currShader = currShader->nextStage;
	} while (currShader->nextStage);

	// prepare the input semantics, this is only required for the vertex shader
	prepareInputs(compilerInfo.vertShader.get());

	// and link the output from each shader stage, with the input of the next
	// inputs for other shader stages will be determined by the output from the previous stage
	prepareOutputs(compilerInfo);

	// finalise the shder code blocks and compile into glsl byte code
	Shader shader(context);

	currShader = compilerInfo.vertShader.get();
	do
	{
		currShader->appendBlock += currShader->code;
		shader.add(currShader->appendBlock, currShader->type);
		currShader = currShader->nextStage;

	} while (currShader->nextStage);

	// now we have all the data required from the shader, create some of the vulkan
	// resources now to save time later
	// Create the descriptor layouts for each set
	for (const DescriptorLayout& layout : program.descrLayouts)
	{
		program.pLineLayout.prepare(context);
	}

	// create the pipeline layout - as we know the descriptor layout and any push blocks
	program.pLineLayout.prepare(context, program.descrLayouts);
}

bool ShaderCompiler::compileStage(ShaderDescriptor* shader)
{

}

// =================== ShaderProgram ======================

void ShaderProgram::addVariant(Util::String definition, uint8_t value, Shader::StageType stage)
{
}

void ShaderProgram::overrideRenderState(RenderStateBlock* renderState)
{
}

void ShaderProgram::updateConstant(Util::String name, uint32_t value, Shader::StageType stage)
{
}

void ShaderProgram::updateConstant(Util::String name, int32_t value, Shader::StageType stage)
{
}

void ShaderProgram::updateConstant(Util::String name, float value, Shader::StageType stage)
{
}

bool ShaderProgram::prepare(ShaderParser& parser)
{
	ShaderCompiler compiler(*this);

	// create a variation of the shader if variants are specfied
	if (!variants.empty())
	{
		compiler.addVariant(variants);
	}

	if (!compiler.compile(parser))
	{
		return false;
	}
	return true;
}

void ShaderProgram::prepareStage(ShaderDescriptor* shader, VariantMap& variantDefines)
{
	ShaderCompiler compiler(*this);

	if (!variantDefines.empty())
	{
		compiler.addVariant(variantDefines);
	}

	if (!compiler.compileStage(shader))
	{
		return false;
	}
	return true;
}

// =================== Shader Manager ==================
ShaderManager::ShaderManager(VkDriver& context)
    : context(context)
{
}

ShaderManager::~ShaderManager()
{
	for (auto& program : programs)
	{
		if (program.second)
		{
			delete program.second;
		}
	}
}

ShaderProgram* ShaderManager::createNewInstance(Util::String name, RenderStateBlock* renderBlock, uint64_t variantBits)
{
	ShaderProgram* instance = new ShaderProgram();

	ShaderHash hash{ name.c_str(), variantBits, renderBlock };
	programs.emplace(hash, instance);
	return instance;
}

bool ShaderManager::hasShaderVariant(Util::String name, RenderStateBlock* renderBlock, uint64_t variantBits)
{
	ShaderHash hash{ name.c_str(), variantBits, renderBlock };
	auto iter = programs.find(hash);
	if (iter == programs.end())
	{
		return false;
	}
	return true;
}

ShaderDescriptor* ShaderManager::createCachedInstance(Util::String name, RenderStateBlock* renderBlock,
                                                      uint64_t variantBits)
{
	ShaderDescriptor* instance = new ShaderDescriptor();

	ShaderHash hash{ name.c_str(), variantBits, renderBlock };
	cached.emplace(hash, instance);
	return instance;
}

bool ShaderManager::hasShaderVariantCached(Util::String name, RenderStateBlock* renderBlock, uint64_t variantBits)
{
	ShaderHash hash{ name.c_str(), variantBits, renderBlock->rastState };
	auto iter = cached.find(hash);
	if (iter == cached.end())
	{
		return false;
	}
	return true;
}

}    // namespace VulkanAPI
