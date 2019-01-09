#include "Shader.h"
#include "utility/logger.h"
#include "Vulkan/Descriptors.h"
#include "Vulkan/Pipeline.h"

#include "spirv_cross.hpp"
#include <fstream>

namespace VulkanAPI
{
	Shader::Shader()
	{

	}

	Shader::~Shader()
	{
	}

	static vk::ShaderStageFlagBits get_stage_flag_bits(StageType type)
	{
		vk::ShaderStageFlagBits ret;
		switch (type) {
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

	bool Shader::add(vk::Device device, const char* filename, StageType type)
	{
		if (!loadShaderBinary(filename, type)) {
			return false;
		}

		createModule(device, type);
		createWrapper(type);
		return true;
	}

	bool Shader::add(vk::Device device, const char* filename1, StageType type1, const char* filename2, StageType type2)
	{
		if (!add(device, filename1, type1)) {
			return false;
		}
		if (!add(device, filename2, type2)) {
			return false;
		}
		return true;
	}

	bool Shader::loadShaderBinary(const char* filename, StageType type)
	{
		std::string shaderDir("assets/shaders/");
		std::ifstream file(shaderDir + filename, std::ios_base::ate | std::ios_base::binary);
		if (!file.is_open()) {
			return false;
		}

		std::ifstream::pos_type filePos = file.tellg();
		data[(int)type].resize(filePos);
		file.seekg(0, std::ios_base::beg);
		file.read((char*)data.data(), filePos);
		return true;
	}

	void Shader::createModule(vk::Device device, StageType type)
	{
		vk::ShaderModuleCreateInfo shaderInfo({}, static_cast<uint32_t>(data[(int)type].size()), data[(int)type].data());
		
		VK_CHECK_RESULT(device.createShaderModule(&shaderInfo, nullptr, &modules[(int)type]));

	}

	void Shader::createWrapper(StageType type)
	{
		vk::ShaderStageFlagBits stage = get_stage_flag_bits(type);
		vk::PipelineShaderStageCreateInfo createInfo({}, stage, modules[(int)type], "main", nullptr);
		wrappers[(int)type] = createInfo;
	}

	void Shader::reflection(StageType type, DescriptorLayout& descr_layout, PipelineLayout& p_info, Pipeline& pipeline)
	{
		spirv_cross::Compiler compiler(std::move(data[(int)type]));

		auto shader_res = compiler.get_shader_resources();

		// sampler 2D
		for (auto& image : shader_res.sampled_images) {

			uint32_t set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(image.id, spv::DecorationBinding);
			descr_layout.add_layout(binding, vk::DescriptorType::eSampler, get_stage_flag_bits(type));
		}

		// ubo
		for (auto& ubo : shader_res.uniform_buffers) {

			uint32_t set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);
			descr_layout.add_layout(binding, vk::DescriptorType::eUniformBuffer, get_stage_flag_bits(type));
		}

		// storage
		for (auto& ssbo : shader_res.storage_buffers) {

			uint32_t set = compiler.get_decoration(ssbo.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(ssbo.id, spv::DecorationBinding);
			descr_layout.add_layout(binding, vk::DescriptorType::eStorageBuffer, get_stage_flag_bits(type));
		}

		// image storage
		for (auto& image : shader_res.storage_images) {

			uint32_t set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(image.id, spv::DecorationBinding);
			descr_layout.add_layout(binding, vk::DescriptorType::eStorageImage, get_stage_flag_bits(type));
		}

		// get the number of input and output stages
		for (auto& input : shader_res.stage_inputs) {

			uint32_t location = compiler.get_decoration(input.id, spv::DecorationLocation);
			auto& base_type = compiler.get_type(input.base_type_id);
			auto& member = compiler.get_type(base_type.self);

			if (member.vecsize && member.columns == 1) {
				uint32_t vec_size = compiler.type_struct_member_matrix_stride(member, 0);
				
			}
		}
		for (auto& output : shader_res.stage_outputs) {

			uint32_t location = compiler.get_decoration(output.id, spv::DecorationLocation);
		}

		// get push constants struct sizes if any
		if (!shader_res.push_constant_buffers.empty()) {
			p_info.push_constant_sizes[(int)type] = compiler.get_declared_struct_size(compiler.get_type(shader_res.push_constant_buffers.front().base_type_id));
		}
	}
}
