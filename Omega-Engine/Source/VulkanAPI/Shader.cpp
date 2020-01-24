#include "Shader.h"

#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/Sampler.h"
#include "VulkanAPI/VkContext.h"
#include "file.h"
#include "libshaderc_util/io.h"
#include "utility/Logger.h"

namespace VulkanAPI
{

IncludeInterface::~IncludeInterface()
{
}

shaderc_include_result* IncludeInterface::GetInclude(
    const char* requested_source,
    shaderc_include_type include_type,
    const char* requesting_source,
    size_t)
{

    const std::string full_path = (include_type == shaderc_include_type_relative)
        ? fileFinder.FindRelativeReadableFilepath(requesting_source, requested_source)
        : fileFinder.FindReadableFilepath(requested_source);

    if (full_path.empty())
    {
        LOGGER_ERROR("Unable to find or open include file: %s", requested_source);
        return nullptr;
    }

    // Read the file and save its full path and contents into stable addresses.
    FileInfo* new_file_info = new FileInfo {full_path, {}};
    if (!shaderc_util::ReadFile(full_path, &(new_file_info->contents)))
    {
        LOGGER_ERROR("Unable to read include file: %s", requested_source);
        return nullptr;
    }

    includedFiles.insert(full_path);

    return new shaderc_include_result {new_file_info->full_path.data(),
                                       new_file_info->full_path.length(),
                                       new_file_info->contents.data(),
                                       new_file_info->contents.size(),
                                       new_file_info};
}

void IncludeInterface::ReleaseInclude(shaderc_include_result* include_result)
{
    FileInfo* info = static_cast<FileInfo*>(include_result->user_data);
    delete info;
    delete include_result;
}

// ============================================================================================================================

shaderc_shader_kind getShaderKind(const Shader::Type type)
{
    shaderc_shader_kind result;
    switch (type)
    {
        case Shader::Type::Vertex:
            result = shaderc_shader_kind::shaderc_vertex_shader;
            break;
        case Shader::Type::Fragment:
            result = shaderc_shader_kind::shaderc_fragment_shader;
            break;
        case Shader::Type::Geometry:
            result = shaderc_shader_kind::shaderc_geometry_shader;
            break;
        case Shader::Type::Compute:
            result = shaderc_shader_kind::shaderc_compute_shader;
            break;
    }

    return result;
}

GlslCompiler::GlslCompiler(std::string shaderCode, const Shader::Type type)
    : kind(getShaderKind(type)), source(shaderCode), sourceName(OE_SHADER_DIR)
{
}

GlslCompiler::~GlslCompiler()
{
}

bool GlslCompiler::compile(bool optimise)
{
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    for (auto& define : definitions)
    {
        options.AddMacroDefinition(define.first, std::to_string(define.second).c_str());
    }

    if (optimise)
    {
        options.SetOptimizationLevel(shaderc_optimization_level_size);
    }

    options.SetSourceLanguage(shaderc_source_language_glsl);
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);

    // we need to create a new instantation of the includer interface - using the one from shaerc -
    // though could use our own.
    std::unique_ptr<IncludeInterface> includer(new IncludeInterface(&fileFinder));
    const auto& used_source_files = includer->filePathTrace();
    options.SetIncluder(std::move(includer));

    auto result = compiler.CompileGlslToSpv(source, kind, sourceName.c_str(), options);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        LOGGER_INFO("%s", result.GetErrorMessage().c_str());
        return false;
    }

    std::copy(result.cbegin(), result.cend(), back_inserter(output));

    return true;
}

// ==================== Shader =========================

Shader::Shader(VkContext& context, const Type type) : context(context), type(type)
{
}

Shader::~Shader()
{
}

Util::String Shader::shaderTypeToString(Shader::Type type)
{
    Util::String result;
    switch (type)
    {
        case Shader::Type::Vertex:
            result = "Vertex";
            break;
        case Shader::Type::Fragment:
            result = "Fragment";
            break;
        case Shader::Type::TesselationCon:
            result = "TesselationCon";
            break;
        case Shader::Type::TesselationEval:
            result = "TesselationEval";
            break;
        case Shader::Type::Geometry:
            result = "Geometry";
            break;
        case Shader::Type::Compute:
            result = "Compute";
            break;
    }
    return result;
}

vk::ShaderStageFlagBits Shader::getStageFlags(Shader::Type type)
{
    vk::ShaderStageFlagBits ret;
    switch (type)
    {
        case Shader::Type::Vertex:
            ret = vk::ShaderStageFlagBits::eVertex;
            break;
        case Shader::Type::Fragment:
            ret = vk::ShaderStageFlagBits::eFragment;
            break;
        case Shader::Type::TesselationCon:
            ret = vk::ShaderStageFlagBits::eTessellationControl;
            break;
        case Shader::Type::TesselationEval:
            ret = vk::ShaderStageFlagBits::eTessellationEvaluation;
            break;
        case Shader::Type::Geometry:
            ret = vk::ShaderStageFlagBits::eGeometry;
            break;
        case Shader::Type::Compute:
            ret = vk::ShaderStageFlagBits::eCompute;
            break;
    }
    return ret;
}

bool Shader::compile(
    std::string shaderCode, const Shader::Type type, std::vector<VariantInfo>& variants)
{
    if (shaderCode.empty())
    {
        LOGGER_ERROR("There is no shader code to process!");
        return false;
    }

    // compile into bytecode
    GlslCompiler compiler(shaderCode, type);

    // add definitions to compiler
    for (auto& variant : variants)
    {
        compiler.addVariant(variant.definition, variant.value);
    }

    // compile into bytecode ready for wrapping
    if (!compiler.compile(true))
    {
        LOGGER_ERROR("Error trying to compile shader: check code: \n%s\n", shaderCode.c_str());
        return false;
    }

    // create the shader module
    vk::ShaderModuleCreateInfo shaderInfo({}, compiler.getByteSize(), compiler.getData());

    VK_CHECK_RESULT(context.getDevice().createShaderModule(&shaderInfo, nullptr, &module));

    // create the wrapper - this will be used by the pipeline
    vk::ShaderStageFlagBits stage = getStageFlags(type);
    createInfo = vk::PipelineShaderStageCreateInfo({}, stage, module, "main", nullptr);

    return true;
}

vk::Format Shader::getVkFormatFromType(std::string type, uint32_t width)
{
    // TODO: add other base types and widths
    vk::Format format;

    // floats
    if (width == 32)
    {
        if (type == "float")
        {
            format = vk::Format::eR32Sfloat;
        }
        else if (type == "vec2")
        {
            format = vk::Format::eR32G32Sfloat;
        }
        else if (type == "vec3")
        {
            format = vk::Format::eR32G32B32Sfloat;
        }
        else if (type == "vec4")
        {
            format = vk::Format::eR32G32B32A32Sfloat;
        }
        // signed integers
        else if (type == "int")
        {
            format = vk::Format::eR32Sint;
        }
        else if (type == "ivec2")
        {
            format = vk::Format::eR32G32Sint;
        }
        else if (type == "ivec3")
        {
            format = vk::Format::eR32G32B32Sint;
        }
        else if (type == "ivec4")
        {
            format = vk::Format::eR32G32B32A32Sint;
        }
        else
        {
            LOGGER_ERROR("Unsupported Vulkan format type specified: %s", type.c_str());
        }
    }

    return format;
}

uint32_t Shader::getStrideFromType(std::string type)
{
    // TODO: add other base types and widths
    uint32_t size;

    // floats
    if (type == "float" || type == "int")
    {
        size = 4;
    }
    else if (type == "vec2" || type == "ivec2")
    {
        size = 8;
    }
    else if (type == "vec3" || type == "ivec3")
    {
        size = 12;
    }
    else if (type == "vec4" || type == "ivec4")
    {
        size = 16;
    }
    else
    {
        LOGGER_ERROR(
            "Unsupported type specified: %s. Unable to determine stride size.", type.c_str());
    }

    return size;
}

} // namespace VulkanAPI
