#pragma once
#include "Vulkan/Common.h"
#include "Rendering/RenderableTypes.h"
#include "spirv_cross.hpp"

namespace VulkanAPI
{
	// forward declearions
	class Shader;
	enum class StageType;

	class Pipeline
	{
	public:

		Pipeline(vk::Device device);

		void set_raster_cull_mode(vk::CullModeFlags cull_mode);
		void set_raster_front_face(vk::FrontFace front_face);
		void set_raster_depth_clamp(bool state);

		void set_topology(vk::PrimitiveTopology topology);

		void add_colour_attachment(bool blend_factor);
		void add_dynamic_state(vk::DynamicState state);

		void set_depth_state(bool test_state, bool write_state, vk::CompareOp compare = vk::CompareOp::eLessOrEqual);

		void create();
	
	private:

		vk::Device device;

		// everything needeed to build the pipeline
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

		vk::Pipeline pipeline;
	};


	// all the data required to create a pipeline layout
	class PipelineLayout
	{
	public:

		void create(OmegaEngine::RenderTypes type);

	private:

		DescriptorLayout descriptor;

		// number of input and output stages
		std::array<uint8_t, (int)StageType::Count> input_counts = {};
		std::array<uint8_t, (int)StageType::Count> output_counts = {};

		std::array<uint32_t, (int)StageType::Count> push_constant_sizes = {};

		vk::PipelineLayout layout;
	};

	class PipelineInterface
	{

	public:

		PipelineInterface(vk::Device device);
		~PipelineInterface();

		void shader_reflection(Shader& shader, StageType type, PipelineLayout& p_info);
		void add_shader(OmegaEngine::RenderTypes type);

	private:

		vk::Device device;

		std::array<Shader, (int)OmegaEngine::RenderTypes::Count> shaders;

		std::array<PipelineLayout, (int)OmegaEngine::RenderTypes::Count> pipeline_layouts;
		std::array<Pipeline, (int)OmegaEngine::RenderTypes::Count> pipeline_layouts;
	};

}
