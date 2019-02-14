#include "Pipeline.h"
#include "Vulkan/Shader.h"
#include "Vulkan/Descriptors.h"

namespace VulkanAPI
{
	
	PipelineLayout::PipelineLayout()
	{

	}

	void PipelineLayout::create(vk::Device& device, 
								std::vector<std::tuple<uint32_t, 
								vk::DescriptorSetLayout> >& descr_layout, 
								OmegaEngine::RenderTypes type)
	{	
		// create push constants
		std::vector<vk::PushConstantRange> push_constants;

		for (uint16_t stage = 0; stage < (uint16_t)VulkanAPI::StageType::Count; ++stage) {
			if (push_constant_sizes[stage]) {
				vk::PushConstantRange push(Shader::get_stage_flag_bits((StageType)stage), 0, push_constant_sizes[stage]);
				push_constants.push_back(push);
			}
		}
		
		// the descriptor layout also contains the set number for this layout as derived from the pipelinelayout. 
		// We are not using the set number yet, so just convert the layouts into a format friendly for creating the pipeline layout
		std::vector<vk::DescriptorSetLayout> layouts;
		for (auto& layout : descr_layouts) {
			layouts.push_back(layout.second);
		}

		vk::PipelineLayoutCreateInfo pipelineInfo({},
			static_cast<uint32_t>(descr_layout.size()), descr_layout.data(),
			static_cast<uint32_t>(push_constants.size()), push_constants.data());

		VK_CHECK_RESULT(device.createPipelineLayout(&pipelineInfo, nullptr, &layout));
	}

	
	Pipeline::Pipeline()
	{
		// setup defualt pipeline states
		add_dynamic_state(vk::DynamicState::eScissor);
		add_dynamic_state(vk::DynamicState::eViewport);
		set_topology(vk::PrimitiveTopology::eTriangleList);
	}

	Pipeline::~Pipeline()
	{

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

	void Pipeline::create(vk::Device dev, PipelineType _type)
	{
		device = dev;
		type = _type;
		
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

		VK_CHECK_RESULT(device.createGraphicsPipelines({}, 1, &createInfo, nullptr, &pipeline));
	}

}
