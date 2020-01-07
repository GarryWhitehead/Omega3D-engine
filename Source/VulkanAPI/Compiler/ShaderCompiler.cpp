#include "ShaderCompiler.h"

#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Pipeline.h"

#include "VulkanAPI/VkUtils/StringToVk.h"
#include "VulkanAPI/VkUtils/VkToString.h"

namespace VulkanAPI
{

ShaderCompiler::ShaderCompiler(ShaderProgram& program, VkContext& context)
    : context(context), program(program)
{
}

ShaderCompiler::~ShaderCompiler()
{
}

void ShaderCompiler::prepareBindings(ShaderDescriptor* shader, ShaderBinding& binding, uint16_t& bind)
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

			VkUtils::createVkShaderInput(sampler.name, sampler.type, bind, sampler.groupId, inputLine);
			shader->appendBlock += inputLine + "\n";

			// store the binding data for vk descriptor creation
            DescriptorLayout layout = program.descrPool->createLayout(sampler.groupId, bind, vk::DescriptorType::eCombinedImageSampler, Shader::getStageFlags(shader->type));
            
            // add to the binding information
            vk::DescriptorType type = VkUtils::getVkDescrTypeFromStr(sampler.type);
            ShaderBinding::SamplerBinding sBind { sampler.name, bind, sampler.groupId, type };
            binding.samplerBindings.emplace_back(sBind);
		}
	}

	// uniform buffers
	if (!shader->ubos.empty())
	{
		for (auto& buffer : shader->ubos)
		{
			std::string inputLine;
			uint32_t bufferSize;

            VkUtils::createVkShaderBuffer(buffer.descr.name, buffer.descr.type, buffer.data, bind, buffer.descr.groupId, inputLine, bufferSize);
			shader->appendBlock += inputLine + "\n";

            vk::DescriptorType descrType = VkUtils::getVkDescrTypeFromStr(buffer.descr.type);

			// add the layout to the descriptors
            DescriptorLayout layout = program.descrPool->createLayout(buffer.descr.groupId, bind, descrType, Shader::getStageFlags(shader->type));
            
            // add to the binding information
            vk::DescriptorType type = VkUtils::getVkDescrTypeFromStr(buffer.descr.type);
            ShaderBinding::BufferBinding bBind {buffer.descr.name, bind, buffer.descr.groupId, bufferSize, type };
            binding.bufferBindings.emplace_back(bBind);
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
            program.pLineLayout->addPushConstant(shader->type, bufferSize);
		}
	}

	// specialisation constants
	if (!shader->constants.empty())
	{
		uint16_t constantId = 0;
		for (const auto& constant : shader->constants)
		{
			// inject constant text into temp shader text block
			shader->appendBlock += "layout (constant_id = " + std::to_string(constantId) + ") const " + constant.type + " " + constant.name + " = " + constant.value + "\n";

            binding.constants.emplace_back(ShaderBinding::SpecConstantBinding{constant.name, constantId});
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
        
        vk::Format format = Shader::getVkFormatFromType(input.type, 32);
        uint32_t stride = Shader::getStrideFromType(input.type);
        ShaderProgram::InputBinding iBind { loc, stride, format };
        program.inputs.emplace_back(iBind);
    }
}

void ShaderCompiler::prepareOutputs(ShaderParser& compilerInfo, ShaderBinding& binding)
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
            uint16_t loc = 0;
            for (auto& output : descr->outputs)
            {
                std::string inputLine = "layout (location = " + std::to_string(loc) + ") out " + output.type;
                descr->appendBlock += inputLine + "\n";

                vk::Format format = Shader::getVkFormatFromType(output.type, 32);
                ShaderBinding::RenderTarget rTarget { loc++, format };
                binding.renderTargets.emplace_back(rTarget);
            }
		}
	}
}

bool ShaderCompiler::compile(ShaderParser& compilerInfo)
{
	uint16_t bindCount = 0;

	// prepare the bindings for each stage
	for (auto& descr : compilerInfo.descriptors)
	{
        Shader::Type type = descr->type;
        ShaderBinding binding(context);
        
        prepareBindings(descr.get(), binding, bindCount);
        
        // prepare the input semantics, this is only required for the vertex shader
        if (type == Shader::Type::Vertex)
        {
            prepareInputs(descr.get());
        }
        
        // and link the output from each shader stage, with the input of the next
        // inputs for other shader stages will be determined by the output from the previous stage
        prepareOutputs(compilerInfo, binding);
        
        program.stages.emplace_back(std::move(binding));
	}

	// finalise the shder code blocks and compile into glsl byte code
	for (auto& descr : compilerInfo.descriptors)
	{
		ShaderBinding& binding = program.stages[descr->type];
        std::vector<VulkanAPI::Shader::VariantInfo> stageVariants = program.sortVariants(descr->type);
        binding.shader->compile(descr->code, descr->type, stageVariants);
	};

	// now we have all the data required from the shader, create some of the vulkan
	// resources now to save time later
	// Create the descriptor layouts for each set
    program.descrPool->build();
    program.descrPool->prepareLayouts();

	// create the pipeline layout - as we know the descriptor layout and any push blocks
    program.pLineLayout->prepare(context, *program.descrPool);
    
    return true;
}

}    // namespace VulkanAPI
