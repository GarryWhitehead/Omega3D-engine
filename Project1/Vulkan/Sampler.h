#pragma once
#include "Vulkan/Common.h"

namespace VulkanAPI
{
	enum class SamplerType
	{
		TrilinearWrap,
		LinearWrap,
		TriLinearClamp,
		LinearClamp,
		Count
	};

	class Sampler
	{

	public:

		Sampler(vk::Device device, SamplerType type);
		~Sampler();

		vk::Sampler& get_sampler()
		{
			return sampler;
		}

	private:

		void create(vk::SamplerAddressMode address_mode, vk::Filter filter, vk::SamplerMipmapMode mipmap_mode, bool compare_op);

		vk::Device device;
		vk::Sampler sampler;
	};

}

