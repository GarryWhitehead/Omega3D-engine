#include "Shader.h"
#include "utility/logger.h"
#include "Vulkan/Descriptors.h"
#include "Vulkan/Pipeline.h"

#include <fstream>

namespace VulkanAPI
{
	Shader::Shader()
	{

	}

	Shader::~Shader()
	{
	}

	vk::ShaderStageFlagBits Shader::get_stage_flag_bits(StageType type)
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

		// keep track of which shaders we have added
		curr_stages[(int)type] = true;

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

		curr_stages[(int)type1] = true;
		curr_stages[(int)type2] = true;

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
		vk::ShaderStageFlagBits stage = get_stage_flag_bits(type);
		vk::PipelineShaderStageCreateInfo createInfo({}, stage, modules[(int)type], "main", nullptr);
		wrappers.push_back(createInfo);
	}

	void Shader::descriptor_buffer_reflect(DescriptorLayout& descr_layout, std::vector<ShaderBufferLayout>& buffer_layout)
	{
		// reflect for each stage that has been setup
		for (uint8_t i = 0; i < (uint8_t)StageType::Count; ++i) {

			if (!curr_stages[i]) {
				continue;
			}

			spirv_cross::Compiler compiler(data[i].data(), data[i].size() / sizeof(uint32_t));

			auto shader_res = compiler.get_shader_resources();

			// ubo
			for (auto& ubo : shader_res.uniform_buffers) {

				uint32_t set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
				uint32_t binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);

				// the descriptor type could be either a normal or dynamic uniform buffer. Dynamic buffers must start with "Dynamic::"
				vk::DescriptorType type = vk::DescriptorType::eUniformBuffer;
				if (ubo.name.find("Dynamic::") != std::string::npos) {
					type = vk::DescriptorType::eUniformBufferDynamic;
				}

				descr_layout.add_layout(set, binding, type, get_stage_flag_bits(StageType(i)));
				uint32_t range = compiler.get_declared_struct_size(compiler.get_type(ubo.base_type_id));
				buffer_layout.push_back({ vk::DescriptorType::eUniformBuffer, binding, set, ubo.name.c_str(), range });
			}

			// storage
			for (auto& ssbo : shader_res.storage_buffers) {

				uint32_t set = compiler.get_decoration(ssbo.id, spv::DecorationDescriptorSet);
				uint32_t binding = compiler.get_decoration(ssbo.id, spv::DecorationBinding);

				// the descriptor type could be either a normal or dynamic storage buffer. Dynamic buffers must start with "Dynamic::"
				vk::DescriptorType type = vk::DescriptorType::eStorageBuffer;
				if (ssbo.name.find("Dynamic::") != std::string::npos) {
					type = vk::DescriptorType::eStorageBufferDynamic;
				}

				descr_layout.add_layout(set, binding, type, get_stage_flag_bits(StageType(i)));
				uint32_t range = compiler.get_declared_struct_size(compiler.get_type(ssbo.base_type_id));
				buffer_layout.push_back({ vk::DescriptorType::eStorageBuffer, binding, set, ssbo.name.c_str(), range });
			}

			// image storage
			for (auto& image : shader_res.storage_images) {

				uint32_t set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
				uint32_t binding = compiler.get_decoration(image.id, spv::DecorationBinding);
				descr_layout.add_layout(set, binding, vk::DescriptorType::eStorageImage, get_stage_flag_bits(StageType(i)));
			}
		}
	}

	void Shader::descriptor_image_reflect(DescriptorLayout& descr_layout, std::vector<ShaderImageLayout>& image_layout)
	{
		// reflect for each stage that has been setup
		for (uint8_t i = 0; i < (uint8_t)StageType::Count; ++i) {

			if (!curr_stages[i]) {
				continue;
			}

			spirv_cross::Compiler compiler(data[i].data(), data[i].size() / sizeof(uint32_t));

			auto shader_res = compiler.get_shader_resources();

			// sampler 2D
			for (auto& image : shader_res.sampled_images) {

				uint32_t set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
				uint32_t binding = compiler.get_decoration(image.id, spv::DecorationBinding);
				descr_layout.add_layout(set, binding, vk::DescriptorType::eSampler, get_stage_flag_bits(StageType(i)));
				image_layout.push_back({ binding, set, image.name.c_str() });
			}
		}
	}

	void Shader::pipeline_layout_reflect(PipelineLayout& p_info)
	{
		// reflect for each stage that has been setup
		for (uint8_t i = 0; i < (uint8_t)StageType::Count; ++i) {

			if (!curr_stages[i]) {
				continue;
			}

			spirv_cross::Compiler compiler(data[i].data(), data[i].size() / sizeof(uint32_t));

			auto shader_res = compiler.get_shader_resources();

			// get push constants struct sizes if any
			if (!shader_res.push_constant_buffers.empty()) {
				p_info.add_push_constant((StageType)i, compiler.get_declared_struct_size(compiler.get_type(shader_res.push_constant_buffers.front().base_type_id)));
			}
		}
	}

	void Shader::pipeline_reflection(Pipeline& pipeline)
	{
		// reflect for each stage that has been setup
		for (uint8_t i = 0; i < (uint8_t)StageType::Count; ++i) {

			if (!curr_stages[i]) {
				continue;
			}

			spirv_cross::Compiler compiler(data[i].data(), data[i].size() / sizeof(uint32_t));

			auto shader_res = compiler.get_shader_resources();

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
		}
	}
}
