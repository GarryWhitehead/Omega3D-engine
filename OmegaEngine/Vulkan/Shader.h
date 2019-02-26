#pragma once
#include "Vulkan/Common.h"

#include "external/Spirv-Cross/spirv_cross.hpp"

namespace VulkanAPI
{
	// forward decleartions
	class DescriptorLayout;
	class PipelineLayout;
	class Pipeline;
	enum class StageType;

	// contains useful information in regards to the bindings, sets and names of buffers, samplers, etc.
	// Avoids having to use the descriptor layouts and offers more flexibility
	struct ShaderBufferLayout
	{
		vk::DescriptorType type;
		uint32_t binding;
		uint32_t set;
		uint32_t range;
		const char* name;

		ShaderBufferLayout(vk::DescriptorType _type, uint32_t _bind, uint32_t _set, const char* _name, uint32_t _range) :
			type(_type), binding(_bind), set(_set), name(_name), range(_range)
		{}
	};

	struct ShaderImageLayout
	{
		uint32_t binding;
		uint32_t set;
		const char* name;

		ShaderImageLayout(uint32_t bind, uint32_t s, const char* n) :
			 binding(bind), set(s), name(n)
		{}
	};

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

		static vk::ShaderStageFlagBits get_stage_flag_bits(StageType type);

		bool add(vk::Device device, const char* filename, StageType type);
		bool add(vk::Device device, const char* filename1, StageType type1, const char* filename2, StageType type2);
		void descriptor_buffer_reflect(DescriptorLayout& descr_layout, std::vector<ShaderBufferLayout>& buffer_layout);
		void descriptor_image_reflect(DescriptorLayout& descr_layout, std::vector<ShaderImageLayout>& image_layout);
		void pipeline_layout_reflect(PipelineLayout& p_info);
		void pipeline_reflection(Pipeline& pipeline);

		std::vector<uint32_t> getData(StageType type)
		{
			return data[(int)type];
		}

		std::vector<vk::PipelineShaderStageCreateInfo> get_wrappers()
		{
			return wrappers;
		}

	private:

		bool loadShaderBinary(const char* filename, StageType type);
		void createModule(vk::Device device, StageType type);
		void createWrapper(StageType type);
		
		std::vector<vk::PipelineShaderStageCreateInfo> wrappers;

		std::array<vk::ShaderModule, (int)StageType::Count> modules;
		std::array<std::vector<uint32_t>, (int)StageType::Count > data;
		std::array<bool, (int)StageType::Count > curr_stages = { false };
	};

}

