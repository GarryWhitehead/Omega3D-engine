#include "ShaderCompiler.h"

#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/VkUtils/StringToVk.h"
#include "VulkanAPI/VkUtils/VkToString.h"
#include "utility/FileUtil.h"
#include "utility/Logger.h"

namespace VulkanAPI
{

ShaderCompiler::ShaderCompiler(ShaderProgram& program, VkContext& context)
    : context(context), program(program)
{
}

ShaderCompiler::~ShaderCompiler()
{
}

std::string ShaderCompiler::getErrorString()
{
    // TODO:: make this more verbose
    std::string result = "Shader compiling failed with error code: " +
        std::to_string(static_cast<int>(errorCache.code));
    if (!errorCache.name.empty())
    {
        result += "Failed handling descriptor: " + errorCache.name + " of type: " + errorCache.type;
    }
    return result;
}

void ShaderCompiler::printShaderCode(const std::string& block)
{
    uint32_t line = 0;

    printf("Shader Code Block:\n");
    printf("%i   ", line++);
    for (auto& c : block)
    {
        if (c == '\n')
        {
            printf("\n%i   ", line++);
        }
        else
        {
            printf("%s", c);
        }
    }
    printf("\n\n");
}

bool ShaderCompiler::getBool(std::string type)
{
    if (type == "True")
    {
        return true;
    }
    else if (type == "False")
    {
        return false;
    }

    // rather than return a error code, we just return false.
    return false;
}

CompilerReturnCode ShaderCompiler::preparePipelineBlock(ShaderParser& compilerInfo)
{
    program.renderState = std::make_unique<RenderStateBlock>();

    for (auto& state : compilerInfo.pipelineDescrs)
    {
        RenderStateBlock::DsState& dsState = program.renderState->dsState;
        RenderStateBlock::RasterState rState = program.renderState->rastState;
        RenderStateBlock::Sampler sampler = program.renderState->sampler;

        // depth-stencil state
        if (state.first == "DepthTestEnable")
        {
            dsState.testEnable = getBool(state.second);
        }
        else if (state.first == "DepthWriteEnable")
        {
            dsState.writeEnable = getBool(state.second);
        }
        else if (state.first == "CompareOp")
        {
            dsState.compareOp = VkUtils::vkCompareOpFromString(state.second);
        }
        // raster state
        else if (state.first == "PolygonMode")
        {
            rState.polygonMode = VkUtils::vkPolygonFromString(state.second);
        }
        else if (state.first == "CullMode")
        {
            rState.cullMode = VkUtils::vkCullModeFromString(state.second);
        }
        else if (state.first == "FrontFace")
        {
            rState.frontFace = VkUtils::vkFrontFaceFromString(state.second);
        }
        // sampler state
        else if (state.first == "MagFilter")
        {
            sampler.magFilter = VkUtils::vkFilterToString(state.second);
        }
        else if (state.first == "MinFilter")
        {
            sampler.minFilter = VkUtils::vkFilterToString(state.second);
        }
        else if (state.first == "AddressModeU")
        {
            sampler.addrModeU = VkUtils::vkAddressModeToString(state.second);
        }
        else if (state.first == "AddressModeV")
        {
            sampler.addrModeV = VkUtils::vkAddressModeToString(state.second);
        }
        else if (state.first == "AddressModeW")
        {
            sampler.addrModeW = VkUtils::vkAddressModeToString(state.second);
        }
        else
        {
            return CompilerReturnCode::InvalidPipelineType;
        }
    }

    return CompilerReturnCode::Success;
}


CompilerReturnCode
ShaderCompiler::prepareBindings(ShaderDescriptor* shader, ShaderBinding& binding, uint16_t& bind)
{
    // add the glsl version number
    shader->appendBlock += "#version 450\n\n";

    // include files
    if (!shader->includeFiles.empty())
    {
        for (const auto& file : shader->includeFiles)
        {
            // Note: I think the glsl compiler might need the absolute path - need to check
            shader->appendBlock += "#include " + file + "\n";
        }
        shader->appendBlock += '\n';
    }

    // texture samplers
    if (!shader->samplers.empty())
    {
        for (auto& sampler : shader->samplers)
        {
            std::string name, type, inputLine;
            uint16_t groupId = 0;

            bool result = ShaderDescriptor::getTypeValue("Name", sampler, name);
            result &= ShaderDescriptor::getTypeValue("Type", sampler, type);
            if (!result)
            {
                return CompilerReturnCode::InvalidSampler;
            }

            // not a mandatory variable
            result &= ShaderDescriptor::getTypeValue("GroupId", sampler, groupId);
            VkUtils::createVkShaderSampler(name, type, bind, groupId, inputLine);
            shader->appendBlock += inputLine + "\n";

            // store the binding data for vk descriptor creation
            DescriptorLayout layout = program.descrPool->createLayout(
                groupId,
                bind,
                vk::DescriptorType::eCombinedImageSampler,
                Shader::getStageFlags(shader->type));

            // add to the binding information
            vk::DescriptorType descrType = VkUtils::getVkDescrTypeFromStr(type);
            ShaderBinding::SamplerBinding sBind {name, bind, groupId, descrType};
            binding.samplerBindings.emplace_back(sBind);
        }
        shader->appendBlock += '\n';
    }

    // uniform buffers
    if (!shader->ubos.empty())
    {
        for (auto& buffer : shader->ubos)
        {
            std::string name, type, inputLine;
            uint16_t groupId = 0;
            uint32_t bufferSize;

            bool result = ShaderDescriptor::getTypeValue("Name", buffer.descriptors, name);
            result &= ShaderDescriptor::getTypeValue("Type", buffer.descriptors, type);
            
            if (!result)
            {
                return CompilerReturnCode::InvalidBuffer;
            }

            // not a mandatory component
            ShaderDescriptor::getTypeValue("GroupId", buffer.descriptors, groupId);
            
            VkUtils::createVkShaderBuffer(
                name, type, buffer.items, bind, groupId, inputLine, bufferSize);
            shader->appendBlock += inputLine + ";\n\n";

            vk::DescriptorType descrType = VkUtils::getVkDescrTypeFromStr(type);

            // add the layout to the descriptors
            DescriptorLayout layout = program.descrPool->createLayout(
                groupId, bind, descrType, Shader::getStageFlags(shader->type));

            // add to the binding information
            ShaderBinding::BufferBinding bBind {name, bind, groupId, bufferSize, descrType};
            binding.bufferBindings.emplace_back(bBind);
        }
        shader->appendBlock += '\n';
    }

    // push blocks
    if (!shader->pConstants.empty())
    {
        for (const auto& constant : shader->pConstants)
        {
            std::string name, type, id, inputLine;
            uint32_t bufferSize;

            bool result = ShaderDescriptor::getTypeValue("Name", constant.descriptors, name);
            result &= ShaderDescriptor::getTypeValue("Type", constant.descriptors, type);
            if (!result)
            {
                return CompilerReturnCode::InvalidPushConstant;
            }
            // not mandatory
            ShaderDescriptor::getTypeValue("id", constant.descriptors, id);

            // inject pipeline text into temp string block
            VkUtils::createVkShaderBuffer(name, type, constant.items, 0, 0, inputLine, bufferSize);
            // append to main shader text
            shader->appendBlock += inputLine + id + ";\n\n";
            program.pLineLayout->addPushConstant(shader->type, bufferSize);
        }
        shader->appendBlock += '\n';
    }

    // specialisation constants
    if (!shader->constants.empty())
    {
        std::string name, type, value;
        uint16_t constantId = 0;

        for (const auto& constant : shader->constants)
        {
            bool result = ShaderDescriptor::getTypeValue("Name", constant, name);
            result &= ShaderDescriptor::getTypeValue("Type", constant, type);
            result &= ShaderDescriptor::getTypeValue("Value", constant, value);
            if (!result)
            {
                return CompilerReturnCode::InvalidConstant;
            }

            // inject constant text into temp shader text block
            shader->appendBlock += "layout (constant_id = " + std::to_string(constantId) +
                ") const " + type + " " + name + " = " + value + ";\n\n";

            binding.constants.emplace_back(ShaderBinding::SpecConstantBinding {name, constantId});
        }
        shader->appendBlock += '\n';
    }

    return CompilerReturnCode::Success;
}

CompilerReturnCode
ShaderCompiler::writeInputs(ShaderDescriptor* shader, ShaderDescriptor* nextShader)
{
    assert(shader);
    assert(nextShader);

    uint16_t loc = 0;

    for (auto& output : shader->outputs)
    {
        std::string name, type;
        bool result = ShaderDescriptor::getTypeValue("Name", output, name);
        result &= ShaderDescriptor::getTypeValue("Type", output, type);
        if (!result)
        {
            return CompilerReturnCode::InvalidOutput;
        }

        std::string inputLine =
            "layout (location = " + std::to_string(loc) + ") in " + type + " in" + name;
        std::string outputLine =
            "layout (location = " + std::to_string(loc) + ") out " + type + " out" + name;
        shader->appendBlock += outputLine + ";\n";
        nextShader->appendBlock += inputLine + ";\n";
    }
    shader->appendBlock += '\n';
    nextShader->appendBlock += '\n';

    return CompilerReturnCode::Success;
}

CompilerReturnCode ShaderCompiler::prepareInputs(ShaderDescriptor* vertShader)
{
    if (vertShader->inputs.empty())
    {
        // not an error if there are no inputs into the vertex shader
        return CompilerReturnCode::Success;
    }

    uint16_t loc = 0;
    for (auto& input : vertShader->inputs)
    {
        std::string name, type;
        bool result = ShaderDescriptor::getTypeValue("Name", input, name);
        result &= ShaderDescriptor::getTypeValue("Type", input, type);
        if (!result)
        {
            return CompilerReturnCode::InvalidInput;
        }

        std::string inputLine =
            "layout (location = " + std::to_string(loc) + ") in " + type + " in" + name;
        vertShader->appendBlock += inputLine + ";\n";

        vk::Format format = Shader::getVkFormatFromType(type, 32);
        uint32_t stride = Shader::getStrideFromType(type);
        ShaderProgram::InputBinding iBind {loc, stride, format};
        program.inputs.emplace_back(iBind);
    }
    vertShader->appendBlock += '\n';

    return CompilerReturnCode::Success;
}

CompilerReturnCode ShaderCompiler::prepareOutputs(ShaderParser& compilerInfo)
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
            ShaderBinding& fragBinding = program.findShaderBinding(descr->type);
            for (auto& output : descr->outputs)
            {
                std::string name, type;
                bool result = ShaderDescriptor::getTypeValue("Name", output, name);
                result &= ShaderDescriptor::getTypeValue("Type", output, type);
                if (!result)
                {
                    return CompilerReturnCode::InvalidOutput;
                }

                std::string inputLine =
                    "layout (location = " + std::to_string(loc) + ") out " + type + " out" + name;
                descr->appendBlock += inputLine + ";\n";

                vk::Format format = Shader::getVkFormatFromType(type, 32);
                ShaderBinding::RenderTarget rTarget {loc++, format};
                fragBinding.renderTargets.emplace_back(rTarget);
            }
        }
    }
    return CompilerReturnCode::Success;
}

CompilerReturnCode ShaderCompiler::compileAll(ShaderParser& compilerInfo)
{
    uint16_t bindCount = 0;

    // compile the pipeline block
    CompilerReturnCode ret = preparePipelineBlock(compilerInfo);
    if (ret != CompilerReturnCode::Success)
    {
        return ret;
    }

    // compile the bindings for each stage
    for (auto& descr : compilerInfo.descriptors)
    {
        ShaderBinding binding(context, descr->type);
        prepareBindings(descr.get(), binding, bindCount);

        // prepare the input semantics, this is only required for the vertex shader
        if (descr->type == Shader::Type::Vertex)
        {
            prepareInputs(descr.get());
        }

        program.stages.emplace_back(std::move(binding));
    }

    ret = prepareOutputs(compilerInfo);
    if (ret != CompilerReturnCode::Success)
    {
        return ret;
    }

    // finalise the shder code blocks and compile into glsl byte code
    for (auto& descr : compilerInfo.descriptors)
    {
        // finish off the shader block by adding the actual glsl code!
        if (descr->codeBlock.empty())
        {
            return CompilerReturnCode::MissingCodeBlock;
        }
        descr->appendBlock += descr->codeBlock;

        ShaderBinding& binding = program.findShaderBinding(descr->type);
        std::vector<VulkanAPI::Shader::VariantInfo> stageVariants =
            program.sortVariants(descr->type);

        if (!binding.shader->compile(descr->appendBlock, descr->type, stageVariants))
        {
            printShaderCode(descr->appendBlock);
            return CompilerReturnCode::ErrorCompilingGlsl;
        }
    };

    // now we have all the data required from the shader, create some of the vulkan
    // resources now to save time later
    // Create the descriptor layouts for each set
    if (!program.descrPool->isEmpty())
    {
        program.descrPool->build();
        program.descrPool->prepareLayouts();
    }

    // create the pipeline layout - as we know the descriptor layout and any push blocks
    program.pLineLayout->prepare(context, *program.descrPool);

    return CompilerReturnCode::Success;
}

// the public interface
bool ShaderCompiler::compile(ShaderParser& parser)
{
    CompilerReturnCode ret = compileAll(parser);
    if (ret != CompilerReturnCode::Success)
    {
        // this may have been cached, but to make sure....
        errorCache.code = ret;
        return false;
    }
    return true;
}


} // namespace VulkanAPI
