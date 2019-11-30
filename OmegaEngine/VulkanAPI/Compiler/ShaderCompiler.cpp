#include "ShaderCompiler.h"

#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/VkContext.h"

#include "VulkanAPI/VkUtils/StringToVk.h"
#include "VulkanAPI/VkUtils/VkToString.h"

namespace VulkanAPI
{

ShaderCompiler::ShaderCompiler(ShaderProgram& program, VkContext& context)
    : program(program), context(context)
{
}

ShaderCompiler::~ShaderCompiler()
{
}

void ShaderCompiler::prepareBindings(ShaderDescriptor* shader, uint16_t& bind)
{
	// add the glsl version number
	shader->appendBlock += "#version 450\n";

	// include files
	if (!shader->includeFiles.empty())
	{
		for (const auto& file : shader->includeFiles)
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

			VkUtils::createVkShaderInput(sampler.name, sampler.type, bind, setId, inputLine);
			shader->appendBlock += inputLine + "\n";

			// store the binding data for vk descriptor creation
			program.descrLayout.add(sampler.groupId, bind, vk::DescriptorType::eCombinedImageSampler,
			                        Shader::getStageFlags(shader->type));
		}
	}

	// uniform buffers
	if (!shader->ubos.empty())
	{
		for (auto& buffer : shader->ubos)
		{
			std::string inputLine;
			uint32_t bufferSize;

			VkUtils::createVkShaderBuffer(buffer.name, buffer.type, buffer.data, bind, setId, inputLine, bufferSize);
			shader->appendBlock += inputLine + "\n";

			vk::DescriptorType descrType = getVkDescrTypeFromStr(buffer.type);

			// add the layout to the descriptors
			program.descrLayout.add(buffer.groupId, bind, descrType, Shader::getStageFlags(shader->type));
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
	size_t descrCount = compilerInfo.descriptors.size();

	for (size_t i = 0; i < descrCount; ++i)
	{
		ShaderDescriptor* descr = compilerInfo.descriptors[i].get();
		if (i + 1 < descrCount)
		{
			ShaderDescriptor* nextDescr = compilerInfo.descriptors[i + 1].get();
			writeInputs(descr, nextDescr);
		}
		else if (descr->type == Shader::Type::Fragment && !descr->outputs.empty())
		{
			std::string inputLine = "layout (location = " + std::to_string(loc) + ") out " + output.type;
			descr->appendBlock += inputLine + "\n";

			vk::Format format = Shader::getVkFormatFromType(output.type, output.width);
			ShaderProgram::RenderTarget binding{ loc++, format };
			program.renderTargets.emplace_back(binding);
		}
	}
}

bool ShaderCompiler::compile(ShaderParser& compilerInfo)
{
	uint16_t bind = 0;

	// prepare the bindings for each stage
	for (auto& descr : compilerInfo.descriptors)
	{
		Shader::Type type = descr->type;
		std::unique_ptr<ShaderBinding> shader = std::make_unique<ShaderBinding>();
		shader->prepare(descr.get());
		program.stages[type] = std::move(shader);
	}

	// prepare the input semantics, this is only required for the vertex shader
	prepareInputs(compilerInfo.descriptors[Shader::Type::Vertex].get());

	// and link the output from each shader stage, with the input of the next
	// inputs for other shader stages will be determined by the output from the previous stage
	prepareOutputs(compilerInfo);

	// finalise the shder code blocks and compile into glsl byte code
	for (auto& descr : compilerInfo.descriptors)
	{
		ShaderBinding* binding = program.stages[descr->type].get();
		binding->shader.compile(context, descr->code, descr->type);
	};

	// now we have all the data required from the shader, create some of the vulkan
	// resources now to save time later
	// Create the descriptor layouts for each set
	program.descrLayout.prepare(context.getDevice());

	// create the pipeline layout - as we know the descriptor layout and any push blocks
	program.pLineLayout.prepare(context, program.descrLayout.getLayout());
}

bool ShaderCompiler::compileStage(ShaderDescriptor* shader)
{
}

}    // namespace VulkanAPI