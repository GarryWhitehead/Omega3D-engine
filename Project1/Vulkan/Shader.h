#pragma once
#include "Vulkan/Common.h"

namespace VulkanAPI
{

	class Shader
	{

	public:

		Shader(vk::Device device, const char* filename, vk::ShaderStageFlagBits stage);
		~Shader();

		void loadShaderBinary(const char* filename);
		void createModule(vk::Device device);
		void createWrapper(vk::ShaderStageFlagBits stage);

		std::vector<uint32_t> getData()
		{
			return data;
		}

	private:

		vk::PipelineShaderStageCreateInfo wrapper;
		vk::ShaderModule module;
		std::vector<uint32_t> data;
	};

}

