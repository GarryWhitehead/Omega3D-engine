#include "Pipeline.h"
#include "Vulkan/Shader.h"
#include "Vulkan/Descriptors.h"

namespace VulkanAPI
{
	
	PipelineLayout::PipelineLayout()
	{

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

	Pipeline::Pipeline(vk::Device dev, PipelineType t) :
		device(dev),
		type(t)
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

	void Pipeline::add_colour_attachment(bool blend_factor, uint8_t attach_count)
	{
		for (uint8_t i = 0; i < attach_count; ++i) {
			vk::PipelineColorBlendAttachmentState colour;
			colour.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
			color_attach_state.push_back(colour);
	}

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

	void Pipeline::set_renderpass(vk::RenderPass r_pass)
	{
		assert(r_pass);
		renderpass = r_pass;
	}

	void Pipeline::add_shader(Shader& shader)
	{
		auto wrap = shader.get_wrappers();
		assert(!wrap.empty());
		shaders.reserve(wrap.size());
		std::copy(wrap.begin(), wrap.end(), shaders.begin());
	}

	void Pipeline::add_layout(vk::PipelineLayout pl)
	{
		assert(pl);
		pl_layout = pl;
	}

	void Pipeline::add_empty_layout()
	{
		vk::PipelineLayoutCreateInfo create_info;
		VK_CHECK_RESULT(device.createPipelineLayout(&create_info, nullptr, &pl_layout));
	}

	void Pipeline::create()
	{
		vk::GraphicsPipelineCreateInfo createInfo({}, 
		static_cast<uint32_t>(shaders.size()), shaders.data(),
		&vertex_input_state,
		&assembly_state,
		nullptr,
		&viewport_state,
		&raster_state,
		&multi_sample_state,
		&depth_stencil_state,
		&color_blend_state,
		&dynamic_create_state,
		pl_layout,
		renderpass,
		0, nullptr, -1);

		VK_CHECK_RESULT(device.createGraphicsPipelines(VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline));
	}

}
