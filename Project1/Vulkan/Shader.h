#pragma once
#include "Vulkan/Common.h"

namespace VulkanAPI
{
	static vk::ShaderStageFlagBits get_stage_flag_bits(StageType type);

	enum class StageType
	{
		Vertex,
		Fragment,
		Geometry,
		Compute,
		Count
	};

	class Shader
	{

	public:

		Shader();
		~Shader();

		bool add(vk::Device device, const char* filename, StageType type);
		bool add(vk::Device device, const char* filename1, StageType type1, const char* filename2, StageType type2);
		
		std::vector<uint32_t> getData(StageType type)
		{
			return data[(int)type];
		}

	private:

		bool loadShaderBinary(const char* filename, StageType type);
		void createModule(vk::Device device, StageType type);
		void createWrapper(StageType type);

		std::array<vk::PipelineShaderStageCreateInfo, (int)StageType::Count> wrappers;
		std::array<vk::ShaderModule, (int)StageType::Count> modules;
		std::array<std::vector<uint32_t>, (int)StageType::Count > data;
	};

}

