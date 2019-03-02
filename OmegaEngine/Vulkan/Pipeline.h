#pragma once
#include "Vulkan/Common.h"
#include "Vulkan/Shader.h"
#include "Vulkan/Renderpass.h"
#include "Rendering/RenderableTypes/RenderableBase.h"

namespace VulkanAPI
{
	// forward declearions
	class Shader;
	class RenderPass;
	class PipelineLayout;

	enum class PipelineType
	{
		Graphics,
		Compute
	};

	class Pipeline
	{
	public:

		Pipeline();
		~Pipeline();

		void add_vertex_input(uint32_t location, vk::Format format);

		void set_raster_cull_mode(vk::CullModeFlags cull_mode);
		void set_raster_front_face(vk::FrontFace front_face);
		void set_raster_depth_clamp(bool state);

		void set_topology(vk::PrimitiveTopology topology);

		void add_colour_attachment(bool blend_factor, RenderPass& renderpass);
		void add_dynamic_state(vk::DynamicState state);

		void set_depth_state(bool test_state, bool write_state, vk::CompareOp compare = vk::CompareOp::eLessOrEqual);
		void set_renderpass(RenderPass r_pass);
		void add_shader(Shader& shader);
		void add_layout(vk::PipelineLayout pl);
		void add_empty_layout();

		void create(vk::Device dev, RenderPass& renderpass, Shader& shader, PipelineLayout& layout, PipelineType _type);
		void create(vk::Device dev, PipelineType _type);
	
		PipelineType get_pipeline_type() const
		{
			return type;
		}

		vk::Pipeline& get()
		{
			return pipeline;
		}

	private:

		vk::Device device;

		// everything needeed to build the pipeline
		std::vector<vk::VertexInputAttributeDescription> vertex_attr_descr;
		std::vector<vk::VertexInputBindingDescription> vertex_bind_descr;
		vk::PipelineVertexInputStateCreateInfo vertex_input_state;
		vk::PipelineInputAssemblyStateCreateInfo assembly_state;
		vk::PipelineViewportStateCreateInfo viewport_state;
		vk::PipelineRasterizationStateCreateInfo raster_state;
		vk::PipelineMultisampleStateCreateInfo multi_sample_state;
		std::vector<vk::PipelineColorBlendAttachmentState> color_attach_state;
		vk::PipelineColorBlendStateCreateInfo color_blend_state;
		vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;
		std::vector<vk::DynamicState> dynamic_states;
		vk::PipelineDynamicStateCreateInfo dynamic_create_state;

		Shader shader;
		RenderPass renderpass;
		vk::PipelineLayout pl_layout;
		vk::Pipeline pipeline;

		PipelineType type;
	};


	// all the data required to create a pipeline layout
	class PipelineLayout
	{
	public:

		PipelineLayout();

		void create(vk::Device& device, 
						std::vector<std::tuple<uint32_t,
						vk::DescriptorSetLayout> >& descr_layout);

		vk::PipelineLayout& get()
		{
			return layout;
		}

		void add_push_constant(StageType stage, uint32_t size)
		{
			push_constant_sizes[(int)stage] = size;
		}

	private:

		// usually set through shader reflection
		std::array<uint32_t, (int)StageType::Count> push_constant_sizes = {};

		vk::PipelineLayout layout;
	};

}
