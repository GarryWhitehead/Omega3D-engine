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
								std::vector<std::tuple<uint32_t, vk::DescriptorSetLayout> >& descriptorLayout)
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
		std::vector<vk::DescriptorSetLayout> layouts(descriptorLayout.size());

		for (auto& layout : descriptorLayout) {
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
		addDynamicState(vk::DynamicState::eScissor);
		addDynamicState(vk::DynamicState::eViewport);
		setTopology(vk::PrimitiveTopology::eTriangleList);

		raster_state.lineWidth = 1.0f;
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
		// check for empty vertex
		if (vertex_attr_descr.empty()) {
			vertex_input_state.vertexAttributeDescriptionCount = 0;
			vertex_input_state.pVertexAttributeDescriptions = nullptr;
			vertex_input_state.vertexBindingDescriptionCount = 0;
			vertex_input_state.pVertexBindingDescriptions = nullptr;
			return;
		}

		// first sort the attributes so they are in order of location as when we reflect, we can get the inputs in any order
		std::sort(vertex_attr_descr.begin(), vertex_attr_descr.end(), 
			[](const vk::VertexInputAttributeDescription lhs, const vk::VertexInputAttributeDescription rhs) { return lhs.location < rhs.location; });
		
		// calculate the offset for each location - the size of each location is stored temporarily in the offset elemnt of the struct
		uint32_t next_offset = 0;
		uint32_t current_offset = 0;
		uint32_t totalSize = 0;
		uint32_t attr_count = static_cast<uint32_t>(vertex_attr_descr.size());

		for (uint32_t i = 0; i < attr_count; ++i) {

			next_offset = vertex_attr_descr[i].offset;
			vertex_attr_descr[i].offset = current_offset;
			current_offset += next_offset;
			totalSize += next_offset;
		}

		// assuming just one binding at the moment
		vk::VertexInputBindingDescription bind_descr(0, totalSize, vk::VertexInputRate::eVertex);	// should also support instancing
		vertex_bind_descr.push_back(bind_descr);

		vertex_input_state.vertexAttributeDescriptionCount = attr_count;
		vertex_input_state.pVertexAttributeDescriptions = vertex_attr_descr.data();
		vertex_input_state.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_bind_descr.size());
		vertex_input_state.pVertexBindingDescriptions = vertex_bind_descr.data();
	}

	void Pipeline::setRasterCullMode(vk::CullModeFlags cull_mode)
	{
		raster_state.cullMode = cull_mode;
	}

	void Pipeline::setRasterFrontFace(vk::FrontFace front_face)
	{
		raster_state.frontFace = front_face;
	}

	void Pipeline::set_raster_depth_clamp(bool state)
	{
		raster_state.depthBiasClamp = state;
	}

	void Pipeline::setTopology(vk::PrimitiveTopology topology)
	{
		assembly_state.topology = topology;
	}

	void Pipeline::addColourAttachment(bool blend_factor, RenderPass& renderpass)
	{
		for (uint32_t i = 0; i < renderpass.get_attach_count(); ++i) {
			vk::PipelineColorBlendAttachmentState colour;
			colour.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
			colour.blendEnable = blend_factor;
			color_attach_state.push_back(colour);
	}

		if (blend_factor == VK_TRUE) {

			// deal with blending here
		}

		// update blend state
		color_blend_state.attachmentCount = static_cast<uint32_t>(color_attach_state.size());
		color_blend_state.pAttachments = color_attach_state.data();
	}

	void Pipeline::addDynamicState(vk::DynamicState state)
	{
		// this needs to be enabled if using depth bias - TODO: should also check this is supported by the hardware
		if (state == vk::DynamicState::eDepthBias)
		{
			raster_state.depthBiasEnable = VK_TRUE;
		}

		dynamic_states.push_back(state);
		dynamic_create_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
		dynamic_create_state.pDynamicStates = dynamic_states.data();
	}

	void Pipeline::setDepthState(bool test_state, bool write_state, vk::CompareOp compare)
	{
		depth_stencil_state.depthTestEnable = test_state;
		depth_stencil_state.depthWriteEnable = write_state;
		depth_stencil_state.depthCompareOp = compare;
	}

	void Pipeline::set_renderpass(RenderPass r_pass)
	{
		renderpass = r_pass;
	}

	void Pipeline::addShader(Shader& shader)
	{
		this->shader = shader;
	}

	void Pipeline::add_layout(vk::PipelineLayout pl)
	{
		assert(pl);
		pipelineLayout = pl;
	}

	void Pipeline::add_empty_layout()
	{
		vk::PipelineLayoutCreateInfo create_info;
		VK_CHECK_RESULT(device.createPipelineLayout(&create_info, nullptr, &pipelineLayout));
	}

	void Pipeline::create(vk::Device dev, RenderPass& renderpass, Shader& shader, PipelineLayout& layout, PipelineType _type)
	{
		device = dev;
		type = _type;
		this->renderpass = renderpass;
		this->pipelineLayout = layout.get();

		// calculate the offset and stride size 
		update_vertex_input();

		// use the image size form the renderpass to construct the viewport. Will probably want to offer more methods in the future?
		vk::Viewport view_port(0.0f, 0.0f, (float)renderpass.getImage_width(), (float)renderpass.getImage_height(), 0.0f, 1.0f);
		vk::Rect2D scissor(vk::Offset2D(0, 0), vk::Extent2D((uint32_t)view_port.width, (uint32_t)view_port.height));
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
		pipelineLayout,
		this->renderpass.get(),
		0, nullptr, 0);

		VK_CHECK_RESULT(device.createGraphicsPipelines({}, 1, &createInfo, nullptr, &pipeline));
	}


	void Pipeline::create(vk::Device dev, PipelineType _type)
	{
		device = dev;
		type = _type;

		// calculate the offset and stride size 
		update_vertex_input();

		// use the image size form the renderpass to construct the viewport. Will probably want to offer more methods in the future?
		vk::Viewport view_port(0.0f, 0.0f, (float)renderpass.getImage_width(), (float)renderpass.getImage_height(), 0.0f, 1.0f);
		vk::Rect2D scissor(vk::Offset2D(0, 0), vk::Extent2D((uint32_t)view_port.width, (uint32_t)view_port.height));
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
				pipelineLayout,
				this->renderpass.get(),
				0, nullptr, 0);

		VK_CHECK_RESULT(device.createGraphicsPipelines({}, 1, &createInfo, nullptr, &pipeline));
	}

}
