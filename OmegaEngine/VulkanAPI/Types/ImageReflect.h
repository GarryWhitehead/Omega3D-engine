#pragma once

#include "spirv_cross.hpp"

#include <optional>
#include <vector>

namespace VulkanAPI
{

struct ImageReflect
{
public:
	
	struct ImageLayout
	{
		uint32_t binding;
		uint32_t set;
		Util::String name;
		vk::ImageLayout layout;
		vk::DescriptorType type;
		Sampler sampler;

		ImageLayout() = default;

		ImageLayout(vk::DescriptorType _type, vk::ImageLayout lo, uint32_t bind, uint32_t s, std::string n, Sampler spl)
		    : type(_type)
		    , layout(lo)
		    , binding(bind)
		    , set(s)
		    , name(n)
		    , sampler(spl)
		{
		}
	};

	void prepare(uint32_t* data, size_t dataSize, DescriptorLayout& descriptorLayout)
	{
		spirv_cross::Compiler compiler(data, dataSize / sizeof(uint32_t));

		auto shaderResources = compiler.get_shader_resources();

		// combined image sampler 2D
		for (auto& image : shaderResources.sampled_images)
		{
			uint32_t set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(image.id, spv::DecorationBinding);

			Sampler sampler = getSamplerType(image.name);

			// the image layout can also be set via the sampler name - Depth:: for depth sampler, Colour:: for colour samplers (default if none found)
			vk::ImageLayout imageLayout = getImageLayout(image.name);

			descriptorLayout.addLayout(set, binding, vk::DescriptorType::eCombinedImageSampler,
			                           getStageFlags(StageType(i)));
			imageReflect.layouts.push_back(
			    { vk::DescriptorType::eCombinedImageSampler, imageLayout, binding, set, image.name, sampler });
		}

		// make sure that the samplers bindings are sorted into ascending order - spirv cross seems to mess the order up
		std::sort(imageReflect.layouts.begin(), imageReflect.layouts.end(),
		          [](const ImageReflection::ShaderImageLayout lhs, const ImageReflection::ShaderImageLayout rhs) {
			          return lhs.binding < rhs.binding;
		          });
	}
	

	std::optional<ImageLayout> find(uint32_t set, uint32_t binding)
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

	friend class VulkanAPI::VkTextureManager;

private:

	std::vector<ImageLayout> layouts;
};

}    // namespace VulkanAPI