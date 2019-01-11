#pragma once
#include "Vulkan/Common.h"

namespace VulkanAPI
{
	// forward decleartions
	class DescriptorLayout;
	class PipelineLayout;
	
	static vk::ShaderStageFlagBits get_stage_flag_bits(StageType type);

	// contains useful information in regards to the bindings, sets and names of buffers, samplers, etc.
	// Avoids having to use the descriptor layouts and offers more flexibility
	struct ShaderBufferLayout
	{
		enum LayoutType
		{
			UniformBuffer,
			StorageBuffer,
		};

		LayoutType type;
		uint8_t binding;
		uint8_t set;
		const char* name;

		ShaderBufferLayout(LayoutType t, uint8_t bind, uint8_t s, const char* n) :
			type(t), binding(bind), set(s), name(n)
		{}
	};

	struct ShaderImageLayout
	{
		uint8_t binding;
		uint8_t set;
		const char* name;

		ShaderImageLayout(uint8_t bind, uint8_t s, const char* n) :
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

		bool add(vk::Device device, const char* filename, StageType type);
		bool add(vk::Device device, const char* filename1, StageType type1, const char* filename2, StageType type2);
		void reflection(StageType type, DescriptorLayout& descr_layout, PipelineLayout& p_info, Pipeline& pipeline, std::vector<ShaderImageLayout>& image_layout, std::vector<ShaderBufferLayout>& buffer_layout);
		
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

