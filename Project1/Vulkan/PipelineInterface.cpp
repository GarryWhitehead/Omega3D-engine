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

	void PipelineInterface::shader_reflection(Shader& shader, StageType type, PipelineLayout& p_info)
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

	void PipelineLayout::create(OmegaEngine::RenderTypes type)
	{
		// create push constants
		std::vector<vk::PushConstantRange> push_constants;

		for (uint16_t r_type = 0; r_type < (uint16_t)OmegaEngine::RenderTypes::Count; ++r_type) {
			if (push_constant_sizes[r_type]) {
				vk::PushConstantRange push(get_stage_flag_bits((StageType)r_type), 0, push_constant_sizes[r_type]);
				push_constants.push_back(push);
			}
		}
		
		vk::PipelineLayoutCreateInfo pipelineInfo({},
			1, &m_materials[0].descriptor->layout,
			static_cast<uint32_t>(push_constants.size()), push_constants.data());

		VK_CHECK_RESULT(device.createPipelineLayout(&pipelineInfo, nullptr, &pipeline_layouts[(int)type]));
	}

	Pipeline::Pipeline(vk::Device dev) :
		device(dev)
	{
		// setup defualt pipeline states
		add_dynamic_state(vk::DynamicState::eScissor);
		add_dynamic_state(vk::DynamicState::eViewport);
		set_topology(vk::PrimitiveTopology::eTriangleList);
	}

	void Pipeline::set_raster_cull_mode(vk::CullModeFlags cull_mode)
	{
		raster_state.cullMode = cull_mode;
	}

	void Pipeline::set_raster_front_face(vk::FrontFace front_face)
	{
		raster_state.frontFace = front_face;
	}

	void Pipeline::set_raster_depth_clamp(bool state)
	{
		raster_state.depthBiasClamp = state;
	}

	void Pipeline::set_topology(vk::PrimitiveTopology topology)
	{
		assembly_state.topology = topology;
	}

	void Pipeline::add_colour_attachment(bool blend_factor)
	{
		vk::PipelineColorBlendAttachmentState colour;
		colour.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		color_attach_state.push_back(colour);

		if (blend_factor == VK_TRUE) {

			// deal with blending here
		}

		// update blend state
		color_blend_state.attachmentCount = static_cast<uint32_t>(color_attach_state.size());
		color_blend_state.pAttachments = color_attach_state.data();
	}

	void Pipeline::add_dynamic_state(vk::DynamicState state)
	{
		dynamic_states.push_back(state);
		dynamic_create_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
		dynamic_create_state.pDynamicStates = dynamic_states.data();
	}

	void Pipeline::set_depth_state(bool test_state, bool write_state, vk::CompareOp compare)
	{
		depth_stencil_state.depthTestEnable = test_state;
		depth_stencil_state.depthWriteEnable = write_state;
		depth_stencil_state.depthCompareOp = compare;
	}

	void Pipeline::create()
	{
		vk::GraphicsPipelineCreateInfo createInfo({}, 
		2, m_shader.data(),
		&vertex_input_state,
		&assembly_state,
		&viewport_state,
		&raster_state,
		&multi_sample_state,
		&depth_stencil_state,
		&color_blend_state,
		&dynamic_create_state,
		 m_pipelineInfo.layout,
		vkDeferred->GetRenderPass(),
		0;												// render to G-buffer pass
		createInfo.basePipelineIndex = -1;
		createInfo.basePipelineHandle = VK_NULL_HANDLE;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_pipelineInfo.pipeline));
	}

}
