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

uint32_t ShaderCompiler::findDesignator(std::vector<std::string>& buffer, uint32_t startIdx)
{
    uint32_t idx = 0;
    for (size_t i = startIdx, end = buffer.size(); i < end; ++i)
    {
        size_t pos = buffer[i].find("##stage:");
        if (pos != std::string::npos)
        {
            return idx + startIdx;
        }
        ++idx;
    }
    // an error occured if we get here
    return UINT32_MAX;
}

Shader::Type ShaderCompiler::getDesignatorStage(const std::string& line)
{
    if (line.find("vertex") != std::string::npos)
    {
        return Shader::Type::Vertex;
    }
    else if (line.find("fragment") != std::string::npos)
    {
        return Shader::Type::Fragment;
    }
    else if (line.find("geometry") != std::string::npos)
    {
        return Shader::Type::Geometry;
    }
    else if (line.find("tesselationControl") != std::string::npos)
    {
        return Shader::Type::TesselationCon;
    }
    else if (line.find("tesselationEvaluation") != std::string::npos)
    {
        return Shader::Type::TesselationEval;
    }
    return Shader::Type::Unknown;
}


bool ShaderCompiler::appendCodeBlocks(ShaderParser& compilerInfo)
{
    // first, try and read the file into the buffer...
    std::vector<std::string> buffer;
    std::string filename = OE_SHADER_DIR + compilerInfo.codePath;

    if (!FileUtil::readFileIntoBuffer(filename, buffer))
    {
        return false;
    }

    uint32_t lineIdx = 0;

    // find the ##stage: designator for each stage
    for (auto& descr : compilerInfo.descriptors)
    {
        lineIdx = findDesignator(buffer, lineIdx);
        if (lineIdx == UINT32_MAX)
        {
            // not an error nesecerially, the code for this stage may have been added by the
            // individual shader stage
            LOGGER_WARN("Formatting error. No designator found within code block for stage.");
            return true;
        }

        // find the stage, i.e ##stage: fragment
        Shader::Type type = getDesignatorStage(buffer[lineIdx++]);
        if (type == descr->type)
        {
            while (lineIdx < buffer.size())
            {
                // add code to the append block until we hit a ##stagend designator
                if (buffer[lineIdx].find("##stage_end") != std::string::npos)
                {
                    break;
                }
                descr->appendBlock += buffer[lineIdx++];
            }
        }
    }

    return true;
}

void ShaderCompiler::prepareBindings(
    ShaderDescriptor* shader, ShaderBinding& binding, uint16_t& bind)
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

            VkUtils::createVkShaderInput(
                sampler.name, sampler.type, bind, sampler.groupId, inputLine);
            shader->appendBlock += inputLine + ";\n";

            // store the binding data for vk descriptor creation
            DescriptorLayout layout = program.descrPool->createLayout(
                sampler.groupId,
                bind,
                vk::DescriptorType::eCombinedImageSampler,
                Shader::getStageFlags(shader->type));

            // add to the binding information
            vk::DescriptorType type = VkUtils::getVkDescrTypeFromStr(sampler.type);
            ShaderBinding::SamplerBinding sBind {sampler.name, bind, sampler.groupId, type};
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

            VkUtils::createVkShaderBuffer(
                buffer.descr.name,
                buffer.descr.type,
                buffer.data,
                bind,
                buffer.descr.groupId,
                inputLine,
                bufferSize);
            shader->appendBlock += inputLine + ";\n";

            vk::DescriptorType descrType = VkUtils::getVkDescrTypeFromStr(buffer.descr.type);

            // add the layout to the descriptors
            DescriptorLayout layout = program.descrPool->createLayout(
                buffer.descr.groupId, bind, descrType, Shader::getStageFlags(shader->type));

            // add to the binding information
            vk::DescriptorType type = VkUtils::getVkDescrTypeFromStr(buffer.descr.type);
            ShaderBinding::BufferBinding bBind {
                buffer.descr.name, bind, buffer.descr.groupId, bufferSize, type};
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
            VkUtils::createVkShaderBuffer(
                constant.name, constant.type, constant.data, 0, 0, inputLine, bufferSize);
            // append to main shader text
            shader->appendBlock += inputLine + ";\n";
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
            shader->appendBlock += "layout (constant_id = " + std::to_string(constantId) +
                ") const " + constant.type + " " + constant.name + " = " + constant.value + ";\n";

            binding.constants.emplace_back(
                ShaderBinding::SpecConstantBinding {constant.name, constantId});
        }
    }
}

void ShaderCompiler::writeInputs(ShaderDescriptor* shader, ShaderDescriptor* nextShader)
{
    assert(shader);
    assert(nextShader);

    uint16_t loc = 0;

    for (auto& output : shader->outputs)
    {
        std::string inputLine =
            "layout (location = " + std::to_string(loc) + ") in " + output.type + " in" + output.name;
        std::string outputLine =
            "layout (location = " + std::to_string(loc) + ") out " + output.type + " out" + output.name;
        shader->appendBlock += outputLine + ";\n";
        nextShader->appendBlock += inputLine + ";\n";
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
        vertShader->appendBlock += inputLine + ";\n";

        vk::Format format = Shader::getVkFormatFromType(input.type, 32);
        uint32_t stride = Shader::getStrideFromType(input.type);
        ShaderProgram::InputBinding iBind {loc, stride, format};
        program.inputs.emplace_back(iBind);
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
            uint16_t loc = 0;
            ShaderBinding& fragBinding = program.findShaderBinding(descr->type);
            for (auto& output : descr->outputs)
            {
                std::string inputLine =
                    "layout (location = " + std::to_string(loc) + ") out " + output.type + " out" + output.name;
                descr->appendBlock += inputLine + ";\n";

                vk::Format format = Shader::getVkFormatFromType(output.type, 32);
                ShaderBinding::RenderTarget rTarget {loc++, format};
                fragBinding.renderTargets.emplace_back(rTarget);
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
        ShaderBinding binding(context, descr->type);
        prepareBindings(descr.get(), binding, bindCount);

        // prepare the input semantics, this is only required for the vertex shader
        if (descr->type == Shader::Type::Vertex)
        {
            prepareInputs(descr.get());
        }

        program.stages.emplace_back(std::move(binding));
    }

    // and link the output from each shader stage, with the input of the next
    // inputs for other shader stages will be determined by the output from the previous stage
     prepareOutputs(compilerInfo);

    // add the actual code section to the block
    if (!compilerInfo.codePath.empty())
    {
        if (!appendCodeBlocks(compilerInfo))
        {
            return false;
        }
    }

    // finalise the shder code blocks and compile into glsl byte code
    for (auto& descr : compilerInfo.descriptors)
    {
        printf("%s\n", descr->appendBlock.c_str());
        ShaderBinding& binding = program.findShaderBinding(descr->type);
        std::vector<VulkanAPI::Shader::VariantInfo> stageVariants =
            program.sortVariants(descr->type);
        binding.shader->compile(descr->appendBlock, descr->type, stageVariants);
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

} // namespace VulkanAPI
