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

		void createSampler(vk::SamplerAddressMode addressMode, vk::Filter filter, vk::SamplerMipmapMode mipMapMode, bool compare_op);

		vk::Device device;
		vk::Sampler sampler;
	};

}

