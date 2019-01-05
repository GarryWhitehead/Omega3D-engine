#include "PipelineInterface.h"
#include "Vulkan/Shader.h"
#include "Vulkan/Descriptors.h"

namespace VulkanAPI
{

	PipelineInterface::PipelineInterface(vk::Device dev) :
		device(dev)
	{
		// initlaise all shaders that will be used which is dependent on the number of renderable types
		// We could peobably do with some sort of middle man interface here as the renderable types should be seperate from the vulkan backend - maybe?
		for (uint16_t r_type = 0; r_type < (uint16_t)OmegaEngine::RenderTypes::Count; ++r_type) {
			this->add_shader((OmegaEngine::RenderTypes)r_type);
		}
	}


	PipelineInterface::~PipelineInterface()
	{
	}

	void PipelineInterface::shader_reflection(Shader& shader, StageType type, PipelineInfo& p_info)
	{
		std::vector<uint32_t> data = shader.getData(type);
		spirv_cross::Compiler compiler(std::move(data));

		auto shader_res = compiler.get_shader_resources();
		
		// sampler 2D
		for (auto& image : shader_res.sampled_images) {

			uint32_t set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(image.id, spv::DecorationBinding);
			p_info.descriptor.add_layout(binding, vk::DescriptorType::eSampler, get_stage_flag_bits(type));
		}

		// ubo
		for (auto& ubo : shader_res.uniform_buffers) {

			uint32_t set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);
			p_info.descriptor.add_layout(binding, vk::DescriptorType::eUniformBuffer, get_stage_flag_bits(type));
		}

		// storage
		for (auto& ssbo : shader_res.storage_buffers) {

			uint32_t set = compiler.get_decoration(ssbo.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(ssbo.id, spv::DecorationBinding);
			p_info.descriptor.add_layout(binding, vk::DescriptorType::eStorageBuffer, get_stage_flag_bits(type));
		}

		// image storage
		for (auto& image : shader_res.storage_images) {

			uint32_t set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(image.id, spv::DecorationBinding);
			p_info.descriptor.add_layout(binding, vk::DescriptorType::eStorageImage, get_stage_flag_bits(type));
		}

		// get the number of input and output stages
		for (auto& input : shader_res.stage_inputs) {

			p_info.input_counts[(int)type] = compiler.get_decoration(input.id, spv::DecorationLocation);
		}
		for (auto& output : shader_res.stage_outputs) {

			p_info.output_counts[(int)type] = compiler.get_decoration(output.id, spv::DecorationLocation);
		}

		// get push constants struct sizes if any
		if (!shader_res.push_constant_buffers.empty()) {
			p_info.push_constant_sizes[(int)type] = compiler.get_declared_struct_size(compiler.get_type(shader_res.push_constant_buffers.front().base_type_id));
		}
	}

	void PipelineInterface::add_shader(OmegaEngine::RenderTypes type)
	{
		Shader shader;
		switch (type) {
		case OmegaEngine::RenderTypes::Mesh:
			shader.add(device, "models/model.vert", StageType::Vertex, "models/model.frag", StageType::Fragment);
			break;
		case OmegaEngine::RenderTypes::Skybox:
			shader.add(device, "env/skybox.vert", StageType::Vertex, "env/skybox.frag", StageType::Fragment);
			break;
		default:
			LOGGER_INFO("Unsupported render type found whilst initilaising shaders.");
		}

		shaders[(int)type] = shader;
	}

	void PipelineInterface::create_pipeline_layout(PipelineInfo& p_info, OmegaEngine::RenderTypes type)
	{
		// create push constants
		std::vector<vk::PushConstantRange> push_constants;

		for (uint16_t r_type = 0; r_type < (uint16_t)OmegaEngine::RenderTypes::Count; ++r_type) {
			if (p_info.push_constant_sizes[r_type]) {
				vk::PushConstantRange push(get_stage_flag_bits((StageType)r_type), 0, p_info.push_constant_sizes[r_type]);
				push_constants.push_back(push);
			}
		}
		
		vk::PipelineLayoutCreateInfo pipelineInfo({},
			1, &m_materials[0].descriptor->layout,
			static_cast<uint32_t>(push_constants.size()), push_constants.data());

		VK_CHECK_RESULT(device.createPipelineLayout(&pipelineInfo, nullptr, &pipeline_layouts[(int)type]);
	}
}
