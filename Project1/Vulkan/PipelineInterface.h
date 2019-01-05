#pragma once

#include "Rendering/RenderableTypes.h"
#include "spirv_cross.hpp"

namespace VulkanAPI
{
	// forward declearions
	class Shader;
	enum class StageType;

	class PipelineInterface
	{

	public:

		// all the data required to create a pipeline layout
		struct PipelineInfo
		{
			Descriptors descriptor;

			// number of input and output stages
			std::array<uint8_t, (int)StageType::Count> input_counts = {};
			std::array<uint8_t, (int)StageType::Count> output_counts = {};

			std::array<uint32_t, (int)StageType::Count> push_constant_sizes = {};

		};

		PipelineInterface(vk::Device device);
		~PipelineInterface();

		void shader_reflection(Shader& shader, StageType type, PipelineInfo& p_info);
		void add_shader(OmegaEngine::RenderTypes type);

		void create_pipeline_layout(PipelineInfo& p_info, OmegaEngine::RenderTypes type);

	private:

		vk::Device device;

		std::array<Shader, (int)OmegaEngine::RenderTypes::Count> shaders;

		std::array<vk::PipelineLayout, (int)OmegaEngine::RenderTypes::Count> pipeline_layouts;
	};

}
