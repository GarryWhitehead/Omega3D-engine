#include "Sampler.h"

namespace VulkanAPI
{
	Sampler::Sampler()
	{

	}

	Sampler::Sampler(vk::Device dev, SamplerType type) :
		device(dev)
	{
		create(dev, type);
	}

	Sampler::~Sampler()
	{
	}

	void Sampler::create(vk::Device dev, SamplerType type)
	{
		device = dev;

		switch (type) {
		case SamplerType::Clamp:
			create_sampler(vk::SamplerAddressMode::eClampToEdge, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest, false);
			break;
		case SamplerType::Wrap:
			create_sampler(vk::SamplerAddressMode::eRepeat, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest, false);
			break;
		case SamplerType::LinearClamp:
			create_sampler(vk::SamplerAddressMode::eClampToEdge, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest, false);
			break;
		case SamplerType::TriLinearClamp:
			create_sampler(vk::SamplerAddressMode::eClampToEdge, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear, false);
			break;
		case SamplerType::LinearWrap:
			create_sampler(vk::SamplerAddressMode::eRepeat, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest, false);
			break;
		case SamplerType::TrilinearWrap:
			create_sampler(vk::SamplerAddressMode::eRepeat, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear, false);
			break;
		}
	}

	void Sampler::create_sampler(vk::SamplerAddressMode address_mode, vk::Filter filter, vk::SamplerMipmapMode mipmap_mode, bool compare_op)
	{
		vk::SamplerCreateInfo sampler_info({},
			filter, filter,
			mipmap_mode,
			address_mode, address_mode, address_mode,
			0.0f,
			VK_TRUE, 1.0f,			// TODO: add user control of max antriospy
			compare_op ? VK_TRUE : VK_FALSE, vk::CompareOp::eLessOrEqual,
			0.0f, VK_LOD_CLAMP_NONE,			// maxLod should equal the mip-map count?
			vk::BorderColor::eFloatTransparentBlack,
			VK_FALSE);

		VK_CHECK_RESULT(device.createSampler(&sampler_info, nullptr, &sampler));
	}
}
