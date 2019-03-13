#include "Pipeline.h"
#include "Vulkan/Shader.h"
#include "Vulkan/Descriptors.h"
#include "Vulkan/RenderPass.h"

namespace VulkanAPI
{
	
	PipelineLayout::PipelineLayout()
	{

	}

	void PipelineLayout::create(vk::Device& device, 
								std::vector<std::tuple<uint32_t, 
								vk::DescriptorSetLayout> >& descr_layout)
	{	
		// create push constants
		std::vector<vk::PushConstantRange> push_constants;

		for (uint16_t stage = 0; stage < (uint16_t)VulkanAPI::StageType::Count; ++stage) {
			if (push_constant_sizes[stage]) {
				vk::PushConstantRange push(Shader::get_stage_flag_bits((StageType)stage), 0, push_constant_sizes[stage]);
				push_constants.push_back(push);
			}
		}
		
		// the descriptor layout also contains the set number for this layout as derived from the pipelinelayout. The set number number will depict the order which is important
		// as Vulkan will complain otherwise.
		std::vector<vk::DescriptorSetLayout> layouts(descr_layout.size());
		for (auto& layout : descr_layout) {
			layouts[std::get<0>(layout)] = std::get<1>(layout);
		}

		vk::PipelineLayoutCreateInfo pipelineInfo({},
			static_cast<uint32_t>(layouts.size()), layouts.data(),
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

	void Pipeline::add_vertex_input(uint32_t location, vk::Format format, uint32_t size)
	{
		// offsets will be calculated just before pipeline creation
		vk::VertexInputAttributeDescription attr_descr(location, 0, format, size);
		vertex_attr_descr.push_back(attr_descr);
	}

	void Pipeline::update_vertex_input()
	{
		// calculate the offset for each location - the size of each location is stored temporarily in the offset elemnt of the struct
		uint32_t next_offset = 0;
		uint32_t current_offset = 0;
		uint32_t total_size = 0;
		uint32_t attr_count = static_cast<uint32_t>(vertex_attr_descr.size());

		for (uint32_t i = 0; i < attr_count; ++i) {

			next_offset = vertex_attr_descr[i].offset;
			vertex_attr_descr[i].offset = current_offset;
			current_offset = next_offset;
			total_size += next_offset;
		}

		// assuming just one binding at the moment
		vk::VertexInputBindingDescription bind_descr(0, total_size, vk::VertexInputRate::eVertex);	// should also support instancing
		vertex_bind_descr.push_back(bind_descr);

		vertex_input_state.vertexAttributeDescriptionCount = attr_count;
		vertex_input_state.pVertexAttributeDescriptions = vertex_attr_descr.data();
		vertex_input_state.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_bind_descr.size());
		vertex_input_state.pVertexBindingDescriptions = vertex_bind_descr.data();
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

	void Pipeline::add_colour_attachment(bool blend_factor, RenderPass& renderpass)
	{
		for (uint32_t i = 0; i < renderpass.get_attach_count(); ++i) {
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

	void Pipeline::set_renderpass(RenderPass r_pass)
	{
		renderpass = r_pass;
	}

	void Pipeline::add_shader(Shader& shader)
	{
		this->shader = shader;
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

	void Pipeline::create(vk::Device dev, RenderPass& renderpass, Shader& shader, PipelineLayout& layout, PipelineType _type)
	{
		device = dev;
		type = _type;
		this->renderpass = renderpass;
		this->pl_layout = layout.get();

		// calculate the offset and stride size 
		update_vertex_input();

		// use the image size form the renderpass to construct the viewport. Will probably want to offer more methods in the future?
		vk::Viewport view_port(0.0f, 0.0f, renderpass.get_image_width(), renderpass.get_image_height(), 0.0f, 1.0f);
		vk::Rect2D scissor(vk::Offset2D(0, 0), vk::Extent2D(view_port.width, view_port.height));
		viewport_state.pViewports = &view_port;
		viewport_state.viewportCount = 1;
		viewport_state.pScissors = &scissor;
		viewport_state.scissorCount = 1;

		vk::GraphicsPipelineCreateInfo createInfo({}, 
		shader.size(), shader.get_pipeline_data(),
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
		this->renderpass.get(),
		0, nullptr, -1);

		VK_CHECK_RESULT(device.createGraphicsPipelines({}, 1, &createInfo, nullptr, &pipeline));
	}


	void Pipeline::create(vk::Device dev, PipelineType _type)
	{
		device = dev;
		type = _type;

		// calculate the offset and stride size 
		update_vertex_input();

		// use the image size form the renderpass to construct the viewport. Will probably want to offer more methods in the future?
		vk::Viewport view_port(0.0f, 0.0f, renderpass.get_image_width(), renderpass.get_image_height(), 0.0f, 1.0f);
		vk::Rect2D scissor(vk::Offset2D(0, 0), vk::Extent2D(view_port.width, view_port.height));
		viewport_state.pViewports = &view_port;
		viewport_state.viewportCount = 1;
		viewport_state.pScissors = &scissor;
		viewport_state.scissorCount = 1;

		vk::GraphicsPipelineCreateInfo createInfo({},
				shader.size(), shader.get_pipeline_data(),
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
				this->renderpass.get(),
				0, nullptr, -1);

		VK_CHECK_RESULT(device.createGraphicsPipelines({}, 1, &createInfo, nullptr, &pipeline));
	}

}
