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
bool ShaderParser::readShader(rapidjson::Document& doc, ShaderDescriptor& shader, std::string id,
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

	// all samplers, ubos to import
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
					group = std::stoi(import["group"].GetString());
					maxGroup = group;
				}

				if (VkUtils::isSamplerType(name))
				{
					shader.samplers.emplace_back(name, type group);
				}
				else if (VkUtils::isBufferType(name))
				{
					// extra data for a buffer: the types and names of the buffer
					ShaderDescriptor::BufferDescriptor buffer;
					buffer.descr{ name, type, goup };

					const auto& items = import["items"].GetArray();

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
void ShaderCompiler::addVariant(Util::String definition, uint8_t value)
{
}
            
void ShaderCompiler::overrideRenderState(RenderStateBlock& rState)
{

}

void ShaderCompiler::prepareBindings(ShaderParser::ShaderDescriptor* shader, uint16_t& bind)
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

void ShaderCompiler::writeInputs(ShaderParser::ShaderDescriptor* shader, ShaderParser::ShaderDescriptor* nextShader)
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

void ShaderCompiler::prepareInputs(ShaderParser::ShaderDescriptor* vertShader)
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
	ShaderParser::ShaderDescriptor* currShader = compilerInfo.vertShader.get();
	do
	{
		prepareBindings(compilerInfo.vertShader.get(), bind);
		currShader = currShader->nextStage;
	} while (currShader->nextStage);

	// prepare the input semantics, this is only required for the vertex shader
	prepareInputs(compilerInfo.vertShader.get());

	// and link the output from each shader stage, with the input of the next
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
		layout.prepare(context);
	}

	// create the pipeline layout - as we know the descriptor layout and any push blocks
	program.pLineLayout.prepare(context, shaderInfo.descrLayouts);

}



// =================== Shader Manager ==================
ShaderManager::ShaderManager(VkDriver& context)
    : context(context)
{
}

ShaderManager::~ShaderManager()
{
}

ShaderProgram* ShaderManager::findOrCreateShader(Util::String name, ShaderProgram::RenderInfo* renderBlock, uint64_t variantBits, VariantInfo* variantData, uint32_t variantSize)
{
    ShaderProgram* result = nullptr;
    
    // check if a shader with this has already exsists. If it doesn't, then create one.
    auto iter = programs.find({name, variants, render});
    if (iter != programs.end())
    {
        result = &(*iter->second);
    }
    else
    {
        // load the json from file and extract the raw data
        ShaderParser parser;
        if (!parser.parse(name))
        {
            return nullptr;
        }
        
        ShaderCompiler compiler;
        
        // create a variation of the shader if variants are specfied
        if (!variants.empty())
        {
            compiler.addVariant(variants);
        }
        
        // it some instances the user may want to either overwrite the render data that is
        // present in the shader, or explicly specify the data here. One main reason is in
        // the case of materails where the rendering information is obtained from external
        // sources
        if (renderBlock)
        {
            compiler.addRenderData(*renderBlock);
        }
        
        if (!compiler.compile(parser))
        {
            return nullptr;
        }
        ShaderHash hash {name, variants, renderBlock};
        programs.insert(hash, compiler.getProgram());
        result = &programs[hash];
    }
    
    return result;
}

bool ShaderManager::hasShader(Util::String name, RenderStateBlock* renderBlock, uint64_t variantBits)
{
    auto iter = programs.find({name, renderBlock, variants});
    if (iter == programs.end())
    {
        return false;
    }
    return true;
}
            
}    // namespace VulkanAPI
