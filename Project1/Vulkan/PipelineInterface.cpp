#include "PipelineInterface.h"
#include "Vulkan/Shader.h"

namespace VulkanAPI
{

	PipelineInterface::PipelineInterface()
	{
	}


	PipelineInterface::~PipelineInterface()
	{
	}

	void PipelineInterface::shader_reflection(Shader& shader)
	{
		std::vector<uint32_t> data = shader.getData();
		spirv_cross::Compiler compiler(std::move(data));

		Descriptor descriptor;

		auto shader_res = compiler.get_shader_resources();
		
		// sampler 2D
		for (auto& image : shader_res.sampled_images) {

			uint32_t set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(image.id, spv::DecorationBinding);
			
			

		}

		// ubo
		for (auto& ubo : shader_res.uniform_buffers) {

			uint32_t set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);
			spirv_cross::SPIRType base_type = compiler.get_type(ubo.base_type_id);
			
			descriptor.size = compiler.get_declared_struct_size(base_type);
		}

	}
}
