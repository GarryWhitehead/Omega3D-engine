#include "Shader.h"

#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/Sampler.h"

#include "utility/logger.h"

#include <fstream>

namespace VulkanAPI
{

shaderc_shader_kind getShaderKind(const Shader::StageType type)
{
    shaderc_shader_kind result;
    switch (type)
    {
    case Shader::StageType::Vertex:
        result = shaderc_shader_kind::shaderc_vertex_shader;
        break;
    case Shader::StageType::Fragment:
        result = shaderc_shader_kind::shaderc_fragment_shader;
        break;
    case Shader::StageType::Geometry:
        result = shaderc_shader_kind::shaderc_geometry_shader;
        break;
    case Shader::StageType::Compute:
        result = shaderc_shader_kind::shaderc_compute_shader;
        break;
    }

    return result;
}

GlslCompiler::GlslCompiler(std::string shaderCode, const Shader::StageType type) :
    source(shaderCode),
    kind(getShaderKind(type))
{
}

GlslCompiler::~GlslCompiler()
{
}

bool GlslCompiler::compile(bool optimise)
{
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    for (auto define : defines)
    {
        options.AddMacroDefinition(define.first.c_str(), std::to_string(define.second).c_str());
    }

    if (optimise)
    {
        options.SetOptimizationLevel(shaderc_optimization_level_size);
    }

    auto result = compiler.CompileGlslToSpv(source, kind, sourceName.c_str(), options);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        LOGGER_INFO("%s", result.GetErrorMessage().c_str());
        return false;
    }

    std::copy(result.cbegin(), result.cend(), output.begin());

    return true;
}

// ==================== Shader =========================

Shader::Shader(VkDriver& context) :
    context(context)
{
}

Shader::~Shader()
{
}

vk::ShaderStageFlagBits Shader::getStageFlags(Shader::StageType type)
{
	vk::ShaderStageFlagBits ret;
	switch (type)
	{
    case Shader::StageType::Vertex:
		ret = vk::ShaderStageFlagBits::eVertex;
		break;
    case Shader::StageType::Fragment:
		ret = vk::ShaderStageFlagBits::eFragment;
		break;
    case Shader::StageType::TesselationCon:
        ret = vk::ShaderStageFlagBits::eTessellationControl;
        break;
    case Shader::StageType::TesselationEval:
        ret = vk::ShaderStageFlagBits::eTessellationEvaluation;
        break;
    case Shader::StageType::Geometry:
		ret = vk::ShaderStageFlagBits::eGeometry;
		break;
    case Shader::StageType::Compute:
		ret = vk::ShaderStageFlagBits::eCompute;
		break;
	}
	return ret;
}

bool Shader::add(std::string shaderCode, const Shader::StageType type)
{
    if (shaderCode.empty())
    {
        LOGGER_ERROR("There is no shader code to process!");
        return false;
    }
    
    ShaderModuleInfo moduleInfo;
    
    // compile into bytecode
    GlslCompiler compiler(shaderCode, type);
    compiler.addDefinition();
    compiler.compile(true);
    
    // create the shader module
    vk::ShaderModuleCreateInfo shaderInfo({}, compiler.getSize() ,compiler.getData());

    VK_CHECK_RESULT(context.getDevice().createShaderModule(&shaderInfo, nullptr, &moduleInfo.module));
    
    // create the wrapper - this will be used by the pipeline
    vk::ShaderStageFlagBits stage = getStageFlags(type);
    moduleInfo.createInfo = vk::PipelineShaderStageCreateInfo({}, stage, moduleInfo.module, "main", nullptr);
    shaders.push_back(moduleInfo);
}

Sampler Shader::getSamplerType(std::string name)
{
	Sampler sampler;
    vk::Device device = context.getDevice();
    
	// if no sampler declared then will use stock linear sampler
	if (name.find("Clamp_") != std::string::npos)
	{
		sampler.create(device, SamplerType::Clamp);
	}
	if (name.find("Wrap_") != std::string::npos)
	{
		sampler.create(device, SamplerType::Wrap);
	}
	if (name.find("TriLinearWrap_") != std::string::npos)
	{
		sampler.create(device, SamplerType::TrilinearWrap);
	}
	if (name.find("LinearWrap_") != std::string::npos)
	{
		sampler.create(device, SamplerType::LinearWrap);
	}
	if (name.find("TriLinearClamp_") != std::string::npos)
	{
		sampler.create(device, SamplerType::TriLinearClamp);
	}
	if (name.find("LinearClamp_") != std::string::npos)
	{
		sampler.create(device, SamplerType::LinearClamp);
	}
	else
	{
		sampler.create(device, SamplerType::LinearClamp);
	}

	return sampler;
}

vk::ImageLayout Shader::getImageLayout(std::string& name)
{
	vk::ImageLayout layout;
	if (name.find("Depth_") != std::string::npos)
	{
		layout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
		// we can strip the depth identifier from the name
		size_t pos = name.find("Depth_");
		name = name.substr(pos + 6, name.size());
	}
	else if (name.find("Colour_") != std::string::npos)
	{
		layout = vk::ImageLayout::eColorAttachmentOptimal;
		// we can strip the colour identifier from the name
		size_t pos = name.find("Colour_");
		name = name.substr(pos, name.size());
	}
	else
	{
		// default if no identifier found
		layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	}
	return layout;
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
    if (type == "vec2" || type == "ivec2")
    {
        size = 8;
    }
    if (type == "vec3" || type == "ivec3")
    {
        size = 12;
    }
    if (type == "vec4" || type == "ivec4")
    {
        size = 16;
    }

    return size;
}

}    // namespace VulkanAPI
