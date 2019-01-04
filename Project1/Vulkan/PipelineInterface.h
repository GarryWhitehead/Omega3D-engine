#pragma once

#include "spirv_cross.hpp"

namespace VulkanAPI
{

	class PipelineInterface
	{

	public:

		PipelineInterface();
		~PipelineInterface();

		void shader_reflection(Shader& shader);

	private:

	};

}
