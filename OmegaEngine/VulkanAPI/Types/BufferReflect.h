#pragma once

#include "spirv_cross.hpp"

#include <optional>
#include <vector>

namespace VulkanAPI
{

class BufferReflect
{
public:
	// contains useful information in regards to the bindings, sets and names of buffers, samplers, etc.
	// Avoids having to use the descriptor layouts and offers more flexibility
	struct BufferLayout
	{
		vk::DescriptorType type;
		uint32_t binding;
		uint32_t set;
		size_t range;
		std::string name;

		BufferLayout(vk::DescriptorType _type, uint32_t _bind, uint32_t _set, std::string _name, size_t _range)
		    : type(_type)
		    , binding(_bind)
		    , set(_set)
		    , name(_name)
		    , range(_range)
		{
		}
	};

	void prepare(uint32_t* data, size_t dataSize, DescriptorLayout& descriptorLayout)
	{
		spirv_cross::Compiler compiler(data, dataSize / sizeof(uint32_t));
		auto shaderResources = compiler.get_shader_resources();

		// ubo
		for (auto& ubo : shaderResources.uniform_buffers)
		{
			uint32_t set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);

			// the descriptor type could be either a normal or dynamic uniform buffer. Dynamic buffers must start with "Dynamic::"
			vk::DescriptorType type = vk::DescriptorType::eUniformBuffer;
			if (ubo.name.find("Dynamic_") != std::string::npos)
			{
				type = vk::DescriptorType::eUniformBufferDynamic;
			}

			descriptorLayout.addLayout(set, binding, type, getStageFlags(StageType(i)));
			size_t range = compiler.get_declared_struct_size(compiler.get_type(ubo.base_type_id));
			layouts.push_back({ type, binding, set, ubo.name, range });
		}

		// storage
		for (auto& ssbo : shaderResources.storage_buffers)
		{
			uint32_t set = compiler.get_decoration(ssbo.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(ssbo.id, spv::DecorationBinding);

			// the descriptor type could be either a normal or dynamic storage buffer. Dynamic buffers must start with "Dynamic::"
			vk::DescriptorType type = vk::DescriptorType::eStorageBuffer;
			if (ssbo.name.find("Dynamic_") != std::string::npos)
			{
				type = vk::DescriptorType::eStorageBufferDynamic;
			}

			descriptorLayout.addLayout(set, binding, type, getStageFlags(StageType(i)));
			size_t range = compiler.get_declared_struct_size(compiler.get_type(ssbo.base_type_id));
			layouts.push_back({ type, binding, set, ssbo.name, range });
		}

		// image storage
		for (auto& image : shaderResources.storage_images)
		{
			uint32_t set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(image.id, spv::DecorationBinding);
			descriptorLayout.addLayout(set, binding, vk::DescriptorType::eStorageImage,
				                        getStageFlags(StageType(i)));
		}
	}

	std::optional<ShaderBufferLayout> find(uint32_t set, uint32_t binding)
	{
		for (auto& layout : layouts)
		{
			if (layout.set == set && layout.binding == binding)
			{
				return layout;
			}
		}
		return std::nullopt;
	}

private:

	std::vector<BufferLayout> layouts;
};

}    // namespace VulkanAPI