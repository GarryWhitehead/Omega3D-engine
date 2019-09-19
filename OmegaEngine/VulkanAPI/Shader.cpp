#include "Shader.h"
#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/Sampler.h"
#include "utility/logger.h"

#include <fstream>

namespace VulkanAPI
{
Shader::Shader()
{
}

Shader::~Shader()
{
}

vk::ShaderStageFlagBits Shader::getStageFlags(StageType type)
{
	vk::ShaderStageFlagBits ret;
	switch (type)
	{
	case StageType::Vertex:
		ret = vk::ShaderStageFlagBits::eVertex;
		break;
	case StageType::Fragment:
		ret = vk::ShaderStageFlagBits::eFragment;
		break;
	case StageType::Geometry:
		ret = vk::ShaderStageFlagBits::eGeometry;
		break;
	case StageType::Compute:
		ret = vk::ShaderStageFlagBits::eCompute;
		break;
	}
	return ret;
}

bool Shader::add(vk::Device device, const std::string& filename, StageType type)
{
	this->device = device;

	if (!loadShaderBinary(filename.c_str(), type))
	{
		return false;
	}

	createModule(device, type);
	createWrapper(type);

	// keep track of which shaders we have added
	currentStages[(int)type] = true;

	return true;
}

bool Shader::add(vk::Device device, const std::string& filename1, StageType type1, const std::string& filename2,
                 StageType type2)
{
	this->device = device;

	if (!add(device, filename1.c_str(), type1))
	{
		return false;
	}
	if (!add(device, filename2.c_str(), type2))
	{
		return false;
	}

	currentStages[(int)type1] = true;
	currentStages[(int)type2] = true;

	return true;
}

bool Shader::loadShaderBinary(const char* filename, StageType type)
{
	std::string shaderDir(OMEGA_ASSETS_DIR "shaders/");
	std::ifstream file(shaderDir + filename, std::ios_base::ate | std::ios_base::binary);
	if (!file.is_open())
	{
		return false;
	}

	std::ifstream::pos_type filePos = file.tellg();
	data[(int)type].resize(filePos);
	file.seekg(0, std::ios_base::beg);
	file.read((char*)data[(int)type].data(), filePos);
	return true;
}

void Shader::createModule(vk::Device device, StageType type)
{
	vk::ShaderModuleCreateInfo shaderInfo({}, static_cast<uint32_t>(data[(int)type].size()), data[(int)type].data());

	VK_CHECK_RESULT(device.createShaderModule(&shaderInfo, nullptr, &modules[(int)type]));
}

void Shader::createWrapper(StageType type)
{
	vk::ShaderStageFlagBits stage = getStageFlags(type);
	vk::PipelineShaderStageCreateInfo createInfo({}, stage, modules[(int)type], "main", nullptr);
	wrappers.push_back(createInfo);
}


Sampler Shader::getSamplerType(std::string name)
{
	Sampler sampler;

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

std::tuple<vk::Format, uint32_t> Shader::getTypeFormat(uint32_t width, uint32_t vecSize,
                                                       spirv_cross::SPIRType::BaseType type)
{
	// TODO: add other base types and widths
	vk::Format format;
	uint32_t size = 0;

	// floats
	if (type == spirv_cross::SPIRType::Float)
	{
		if (width == 32)
		{
			if (vecSize == 1)
			{
				format = vk::Format::eR32Sfloat;
				size = 4;
			}
			if (vecSize == 2)
			{
				format = vk::Format::eR32G32Sfloat;
				size = 8;
			}
			if (vecSize == 3)
			{
				format = vk::Format::eR32G32B32Sfloat;
				size = 12;
			}
			if (vecSize == 4)
			{
				format = vk::Format::eR32G32B32A32Sfloat;
				size = 16;
			}
		}
	}

	// signed integers
	else if (type == spirv_cross::SPIRType::Int)
	{
		if (width == 32)
		{
			if (vecSize == 1)
			{
				format = vk::Format::eR32Sint;
				size = 4;
			}
			if (vecSize == 2)
			{
				format = vk::Format::eR32G32Sint;
				size = 8;
			}
			if (vecSize == 3)
			{
				format = vk::Format::eR32G32B32Sint;
				size = 12;
			}
			if (vecSize == 4)
			{
				format = vk::Format::eR32G32B32A32Sint;
				size = 16;
			}
		}
	}

	return std::make_tuple(format, size);
}
}    // namespace VulkanAPI
