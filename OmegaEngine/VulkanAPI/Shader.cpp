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

bool Shader::add(vk::Device device, const std::string &filename, StageType type)
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

bool Shader::add(vk::Device device, const std::string &filename1, StageType type1,
                 const std::string &filename2, StageType type2)
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

bool Shader::loadShaderBinary(const char *filename, StageType type)
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
	file.read((char *)data[(int)type].data(), filePos);
	return true;
}

void Shader::createModule(vk::Device device, StageType type)
{
	vk::ShaderModuleCreateInfo shaderInfo({}, static_cast<uint32_t>(data[(int)type].size()),
	                                      data[(int)type].data());

	VK_CHECK_RESULT(device.createShaderModule(&shaderInfo, nullptr, &modules[(int)type]));
}

void Shader::createWrapper(StageType type)
{
	vk::ShaderStageFlagBits stage = getStageFlags(type);
	vk::PipelineShaderStageCreateInfo createInfo({}, stage, modules[(int)type], "main", nullptr);
	wrappers.push_back(createInfo);
}

void Shader::bufferReflection(DescriptorLayout &descriptorLayout, BufferReflection &bufferReflect)
{
	// reflect for each stage that has been setup
	for (uint8_t i = 0; i < (uint8_t)StageType::Count; ++i)
	{

		if (!currentStages[i])
		{
			continue;
		}

		spirv_cross::Compiler compiler(data[i].data(), data[i].size() / sizeof(uint32_t));
		auto shaderResources = compiler.get_shader_resources();

		// ubo
		for (auto &ubo : shaderResources.uniform_buffers)
		{
			uint32_t set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);

			// the descriptor type could be either a normal or dynamic uniform buffer. Dynamic buffers must start with "Dynamic::"
			vk::DescriptorType type = vk::DescriptorType::eUniformBuffer;
			if (ubo.name.find("Dynamic_") != std::string::npos)
			{
				type = vk::DescriptorType::eUniformBufferDynamic;
			}

			descriptorLayout.addLayout(set, binding, type, getStageFlags(StageType(i)));
			size_t range = compiler.get_declared_struct_size(compiler.get_type(ubo.base_type_id));
			bufferReflect.layouts.push_back({ type, binding, set, ubo.name, range });
		}

		// storage
		for (auto &ssbo : shaderResources.storage_buffers)
		{
			uint32_t set = compiler.get_decoration(ssbo.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(ssbo.id, spv::DecorationBinding);

			// the descriptor type could be either a normal or dynamic storage buffer. Dynamic buffers must start with "Dynamic::"
			vk::DescriptorType type = vk::DescriptorType::eStorageBuffer;
			if (ssbo.name.find("Dynamic_") != std::string::npos)
			{
				type = vk::DescriptorType::eStorageBufferDynamic;
			}

			descriptorLayout.addLayout(set, binding, type, getStageFlags(StageType(i)));
			size_t range = compiler.get_declared_struct_size(compiler.get_type(ssbo.base_type_id));
			bufferReflect.layouts.push_back({ type, binding, set, ssbo.name, range });
		}

		// image storage
		for (auto &image : shaderResources.storage_images)
		{
			uint32_t set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(image.id, spv::DecorationBinding);
			descriptorLayout.addLayout(set, binding, vk::DescriptorType::eStorageImage,
			                           getStageFlags(StageType(i)));
		}
	}
}

void Shader::imageReflection(DescriptorLayout &descriptorLayout, ImageReflection &imageReflect)
{
	// reflect for each stage that has been setup
	for (uint8_t i = 0; i < (uint8_t)StageType::Count; ++i)
	{

		if (!currentStages[i])
		{
			continue;
		}

		spirv_cross::Compiler compiler(data[i].data(), data[i].size() / sizeof(uint32_t));

		auto shaderResources = compiler.get_shader_resources();

		// combined image sampler 2D
		for (auto &image : shaderResources.sampled_images)
		{
			uint32_t set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(image.id, spv::DecorationBinding);

			Sampler sampler = getSamplerType(image.name);

			// the image layout can also be set via the sampler name - Depth:: for depth sampler, Colour:: for colour samplers (default if none found)
			vk::ImageLayout imageLayout = getImageLayout(image.name);

			descriptorLayout.addLayout(set, binding, vk::DescriptorType::eCombinedImageSampler,
			                           getStageFlags(StageType(i)));
			imageReflect.layouts.push_back({ vk::DescriptorType::eCombinedImageSampler, imageLayout,
			                                 binding, set, image.name, sampler });
		}

		// make sure that the samplers bindings are sorted into ascending order - spirv cross seems to mess the order up
		std::sort(
		    imageReflect.layouts.begin(), imageReflect.layouts.end(),
		    [](const ImageReflection::ShaderImageLayout lhs,
		       const ImageReflection::ShaderImageLayout rhs) { return lhs.binding < rhs.binding; });
	}
}

void Shader::pipelineLayoutReflect(PipelineLayout &layout)
{
	// reflect for each stage that has been setup
	for (uint8_t i = 0; i < (uint8_t)StageType::Count; ++i)
	{
		if (!currentStages[i])
		{
			continue;
		}

		spirv_cross::Compiler compiler(data[i].data(), data[i].size() / sizeof(uint32_t));

		auto shaderResources = compiler.get_shader_resources();

		// get push constants struct sizes if any
		if (!shaderResources.push_constant_buffers.empty())
		{
			layout.add_push_constant(
			    (StageType)i, (uint32_t)compiler.get_declared_struct_size(compiler.get_type(
			                      shaderResources.push_constant_buffers.front().base_type_id)));
		}
	}
}

void Shader::pipelineReflection(Pipeline &pipeline)
{
	if (!currentStages[(int)StageType::Vertex])
	{
		LOGGER_ERROR("Unable to reflect pipeline as vertex shader has not been initialised.");
	}

	spirv_cross::Compiler compiler(data[(int)StageType::Vertex].data(),
	                               data[(int)StageType::Vertex].size() / sizeof(uint32_t));

	auto shaderResources = compiler.get_shader_resources();

	// get the number of input and output stages
	for (auto &input : shaderResources.stage_inputs)
	{
		uint32_t location = compiler.get_decoration(input.id, spv::DecorationLocation);
		auto &base_type = compiler.get_type(input.base_type_id);
		auto &member = compiler.get_type(base_type.self);

		if (member.vecsize && member.columns == 1)
		{
			std::tuple<vk::Format, uint32_t> info =
			    getTypeFormat(member.width, member.vecsize, member.basetype);
			pipeline.addVertexInput(location, std::get<0>(info), std::get<1>(info));
		}
	}
	/*for (auto& output : shaderResources.stage_outputs) {

			uint32_t location = compiler.get_decoration(output.id, spv::DecorationLocation);
		}*/
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
} // namespace VulkanAPI
