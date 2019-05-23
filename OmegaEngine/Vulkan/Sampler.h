#pragma once
#include "Vulkan/Common.h"

namespace VulkanAPI
{
	enum class SamplerType
	{
		Clamp,
		Wrap,
		TrilinearWrap,
		LinearWrap,
		TriLinearClamp,
		LinearClamp,
		NotDefined
	};

	class Sampler
	{

	public:

		Sampler();
		Sampler(vk::Device device, SamplerType type);
		~Sampler();

		void create(vk::Device dev, SamplerType type);

		vk::Sampler& getSampler()
		{
			return sampler;
		}
		
	private:

		void create_sampler(vk::SamplerAddressMode address_mode, vk::Filter filter, vk::SamplerMipmapMode mipmap_mode, bool compare_op);

		vk::Device device;
		vk::Sampler sampler;
	};

}

