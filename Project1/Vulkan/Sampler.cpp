#include "Sampler.h"

namespace VulkanAPI
{

	Sampler::Sampler(vk::Device dev, SamplerType type) :
		device(dev)
	{
		switch (type) {
		case SamplerType::LinearClamp:
			create(vk::SamplerAddressMode::eClampToEdge, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest, false);
			break;
		case SamplerType::TriLinearClamp:
			create(vk::SamplerAddressMode::eClampToEdge, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear, false);
			break;
		case SamplerType::LinearWrap:
			create(vk::SamplerAddressMode::eRepeat, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest, false);
			break;
		case SamplerType::TrilinearWrap:
			create(vk::SamplerAddressMode::eRepeat, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear, false);
			break;
		}
	}

	Sampler::~Sampler()
	{
	}

	void Sampler::create(vk::SamplerAddressMode address_mode, vk::Filter filter, vk::SamplerMipmapMode mipmap_mode, bool compare_op)
	{
		vk::SamplerCreateInfo sampler_info({},
			filter, filter,
			mipmap_mode,
			address_mode, address_mode, address_mode,
			0.0f,
			VK_TRUE, 1.0f,
			compare_op ? VK_TRUE : VK_FALSE, vk::CompareOp::eLessOrEqual,
			0.0f, VK_LOD_CLAMP_NONE,
			vk::BorderColor::eFloatTransparentBlack,
			VK_FALSE);

		VK_CHECK_RESULT(device.createSampler(&sampler_info, nullptr, &sampler));
	}
}
