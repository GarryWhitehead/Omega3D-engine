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
            printf("%c", c);
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

uint8_t ShaderCompiler::getCurrentBinding(const uint8_t groupId)
{
    uint8_t bind = 0;

    // if the set hasn't been used yet create a new one
    if (currentBinding.empty() || currentBinding.find(groupId) == currentBinding.end())
    {
        currentBinding.emplace(groupId, 1);
    }
    else
    {
        bind = currentBinding[groupId];
        currentBinding[groupId]++;
    }
    return bind;
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

CompilerReturnCode ShaderCompiler::prepareImport(
    ShaderDescriptor& shader,
    ImportType importType,
    ShaderDescriptor::TypeDescriptors& descr,
    ImportInfo& output,
    std::vector<ShaderDescriptor::ItemDescriptors> items)
{
    std::string inputLine;

    bool result = ShaderDescriptor::getTypeValue("Name", descr, output.name);
    result &= ShaderDescriptor::getTypeValue("Type", descr, output.type);
    if (!result)
    {
        return CompilerReturnCode::MissingNameOrType;
    }

    // check whether there is a variant
    ShaderDescriptor::getTypeValue("Variant", descr, output.variant);
    if (!output.variant.empty())
    {
        shader.appendBlock += "#ifdef " + output.variant + "\n";
    }

    // not a mandatory variable
    ShaderDescriptor::getTypeValue("GroupId", descr, output.groupId);
    output.bind = getCurrentBinding(output.groupId);

    if (importType == ImportType::Sampler)
    {
        VkUtils::createVkShaderSampler(
            output.name, output.type, output.bind, output.groupId, inputLine);
        shader.appendBlock += inputLine + "\n";
    }
    else
    {
        if (!VkUtils::createVkShaderBuffer(
                output.name,
                output.type,
                items,
                output.bind,
                output.groupId,
                inputLine,
                output.bufferSize))
        {
            return CompilerReturnCode::InvalidBuffer;
        }

        // not mandatory - adds a sub-id to the buffer
        std::string subId;
        ShaderDescriptor::getTypeValue("id", descr, subId);
        shader.appendBlock += inputLine + subId + ";\n";
    }

    if (!output.variant.empty())
    {
        shader.appendBlock += "#endif\n";
    }

    return CompilerReturnCode::Success;
}

CompilerReturnCode ShaderCompiler::prepareBindings(ShaderDescriptor& shader, ShaderBinding& binding)
{
    // add the glsl version number
    shader.appendBlock += "#version 450\n\n";

    // include files
    if (!shader.includeFiles.empty())
    {
        for (const auto& file : shader.includeFiles)
        {
            // Note: I think the glsl compiler might need the absolute path - need to check
            shader.appendBlock += "#include \"" + file + "\"\n";
        }
        shader.appendBlock += '\n';
    }

    // specialisation constants - order is important here as buffers,et.c might be dependebt on
    // these constants
    if (!shader.constants.empty())
    {
        std::string name, type, value;
        uint16_t constantId = 0;

        for (const auto& constant : shader.constants)
        {
            bool result = ShaderDescriptor::getTypeValue("Name", constant, name);
            result &= ShaderDescriptor::getTypeValue("Type", constant, type);
            result &= ShaderDescriptor::getTypeValue("Value", constant, value);
            if (!result)
            {
                return CompilerReturnCode::InvalidConstant;
            }

            // inject constant text into temp shader text block
            shader.appendBlock += "layout (constant_id = " + std::to_string(constantId++) +
                ") const " + type + " " + name + " = " + value + ";\n\n";

            binding.constants.emplace_back(ShaderBinding::SpecConstantBinding {name, constantId});
        }
    }

    // texture samplers
    if (!shader.samplers.empty())
    {
        for (auto& sampler : shader.samplers)
        {
            ImportInfo importInfo;
            CompilerReturnCode ret =
                prepareImport(shader, ImportType::Sampler, sampler, importInfo);
            if (ret != CompilerReturnCode::Success)
            {
                return ret;
            }

            // store the binding data for vk descriptor creation
            DescriptorLayout layout = program.descrPool->createLayout(
                importInfo.groupId,
                importInfo.bind,
                vk::DescriptorType::eCombinedImageSampler,
                Shader::getStageFlags(shader.type));

            // add to the binding information
            vk::DescriptorType descrType = VkUtils::getVkDescrTypeFromStr(importInfo.type);
            ShaderBinding::SamplerBinding sBind {
                importInfo.name, importInfo.bind, importInfo.groupId, descrType};
            binding.samplerBindings.emplace_back(sBind);
        }
        shader.appendBlock += '\n';
    }

    // uniform buffers
    if (!shader.ubos.empty())
    {
        for (auto& buffer : shader.ubos)
        {
            ImportInfo importInfo;
            CompilerReturnCode ret = prepareImport(
                shader, ImportType::Buffer, buffer.descriptors, importInfo, buffer.items);
            if (ret != CompilerReturnCode::Success)
            {
                return ret;
            }

            // add the layout to the descriptors
            vk::DescriptorType descrType = VkUtils::getVkDescrTypeFromStr(importInfo.type);
            DescriptorLayout layout = program.descrPool->createLayout(
                importInfo.groupId,
                importInfo.bind,
                descrType,
                Shader::getStageFlags(shader.type));

            // add to the binding information
            ShaderBinding::BufferBinding bBind {importInfo.name,
                                                importInfo.bind,
                                                importInfo.groupId,
                                                importInfo.bufferSize,
                                                descrType};

            // check for special buffer attributes e.g. dynamic
            if (importInfo.type == "DynamicUniform")
            {
                bBind.flags |= ShaderBinding::BufferFlags::Dynamic;
            }
            binding.bufferBindings.emplace_back(bBind);
        }
        shader.appendBlock += '\n\n';
    }

    // push blocks
    if (!shader.pConstants.empty())
    {
        for (auto& constant : shader.pConstants)
        {
            ImportInfo importInfo;
            CompilerReturnCode ret = prepareImport(
                shader, ImportType::PushConstant, constant.descriptors, importInfo, constant.items);
            if (ret != CompilerReturnCode::Success)
            {
                return ret;
            }
            program.pLineLayout->addPushConstant(shader.type, importInfo.bufferSize);
        }
        shader.appendBlock += '\n';
    }

    return CompilerReturnCode::Success;
}

CompilerReturnCode
ShaderCompiler::writeInputs(ShaderDescriptor& shader, ShaderDescriptor& nextShader)
{
    uint16_t loc = 0;

    for (auto& output : shader.outputs)
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

        shader.appendBlock += outputLine + ";\n";
        nextShader.appendBlock += inputLine + ";\n";
        ++loc;
    }
    shader.appendBlock += '\n';
    nextShader.appendBlock += '\n';

    return CompilerReturnCode::Success;
}

CompilerReturnCode ShaderCompiler::prepareVertexInputs(ShaderDescriptor& vertShader)
{
    if (vertShader.inputs.empty())
    {
        // not an error if there are no inputs into the vertex shader
        return CompilerReturnCode::Success;
    }

    uint16_t loc = 0;
    for (auto& input : vertShader.inputs)
    {
        std::string name, type;
        bool result = ShaderDescriptor::getTypeValue("Name", input, name);
        result &= ShaderDescriptor::getTypeValue("Type", input, type);
        if (!result)
        {
            return CompilerReturnCode::InvalidInput;
        }

        // check whether there is a variant
        std::string variant;
        ShaderDescriptor::getTypeValue("Variant", input, variant);
        if (!variant.empty())
        {
            vertShader.appendBlock += "#ifdef " + variant + "\n";
        }

        std::string inputLine =
            "layout (location = " + std::to_string(loc) + ") in " + type + " in" + name;
        vertShader.appendBlock += inputLine + ";\n";

        if (!variant.empty())
        {
            vertShader.appendBlock += "#endif\n";
        }

        vk::Format format = Shader::getVkFormatFromType(type, 32);
        uint32_t stride = Shader::getStrideFromType(type);
        ShaderProgram::InputBinding iBind {loc++, stride, format};
        program.inputs.emplace_back(iBind);
    }
    
    vertShader.appendBlock += '\n';

    return CompilerReturnCode::Success;
}

CompilerReturnCode ShaderCompiler::prepareOutputs(ShaderParser& compilerInfo)
{
    size_t descrCount = compilerInfo.descriptors.size();

    for (size_t i = 0; i < descrCount; ++i)
    {
        ShaderDescriptor& descr = compilerInfo.descriptors[i];
        if (i + 1 < descrCount)
        {
            ShaderDescriptor& nextDescr = compilerInfo.descriptors[i + 1];
            writeInputs(descr, nextDescr);
        }
        else if (descr.type == Shader::Type::Fragment && !descr.outputs.empty())
        {
            uint16_t loc = 0;
            ShaderBinding& fragBinding = program.findShaderBinding(descr.type);
            for (auto& output : descr.outputs)
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
                descr.appendBlock += inputLine + ";\n";

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
    // compile the pipeline block
    CompilerReturnCode ret = preparePipelineBlock(compilerInfo);
    if (ret != CompilerReturnCode::Success)
    {
        return ret;
    }

    // compile the bindings for each stage
    for (auto& descr : compilerInfo.descriptors)
    {
        ShaderBinding binding(context, descr.type);
        prepareBindings(descr, binding);

        // prepare the input semantics, this is only required for the vertex shader
        if (descr.type == Shader::Type::Vertex)
        {
            prepareVertexInputs(descr);
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
        if (descr.codeBlock.empty())
        {
            return CompilerReturnCode::MissingCodeBlock;
        }
        descr.appendBlock += descr.codeBlock;

        ShaderBinding& binding = program.findShaderBinding(descr.type);
        std::vector<VulkanAPI::Shader::VariantInfo> stageVariants =
            program.sortVariants(descr.type);

        if (!binding.shader->compile(descr.appendBlock, descr.type, stageVariants))
        {
            printShaderCode(descr.appendBlock);
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
