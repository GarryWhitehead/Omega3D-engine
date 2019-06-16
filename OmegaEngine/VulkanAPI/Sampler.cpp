#include "Sampler.h"
#include "Utility/Logger.h"

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

	SamplerType Sampler::getSamplerType(const vk::SamplerAddressMode mode, const vk::Filter filter)
	{
		SamplerType type;

		if (mode == vk::SamplerAddressMode::eRepeat && filter == vk::Filter::eLinear)
		{
			type = VulkanAPI::SamplerType::LinearWrap;
		}
		if (mode == vk::SamplerAddressMode::eClampToEdge && filter == vk::Filter::eLinear)
		{
			type = VulkanAPI::SamplerType::LinearClamp;
		}
		if (mode == vk::SamplerAddressMode::eClampToEdge && filter == vk::Filter::eNearest)
		{
			type = VulkanAPI::SamplerType::Clamp;
		}
		if (mode == vk::SamplerAddressMode::eRepeat && filter == vk::Filter::eLinear)
		{
			type = VulkanAPI::SamplerType::Wrap;
		}
		else
		{
			LOGGER_INFO("Note: Unsupported sampler type requested.");
		}
		return type;
	}

	void Sampler::create(vk::Device dev, SamplerType type)
	{
		device = dev;

		switch (type) 
		{
			case SamplerType::Clamp:
				createSampler(vk::SamplerAddressMode::eClampToEdge, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest, false);
				break;
			case SamplerType::Wrap:
				createSampler(vk::SamplerAddressMode::eRepeat, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest, false);
				break;
			case SamplerType::LinearClamp:
				createSampler(vk::SamplerAddressMode::eClampToEdge, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest, false);
				break;
			case SamplerType::TriLinearClamp:
				createSampler(vk::SamplerAddressMode::eClampToEdge, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear, false);
				break;
			case SamplerType::LinearWrap:
				createSampler(vk::SamplerAddressMode::eRepeat, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest, false);
				break;
			case SamplerType::TrilinearWrap:
				createSampler(vk::SamplerAddressMode::eRepeat, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear, false);
				break;
		}
	}

	void Sampler::createSampler(vk::SamplerAddressMode addressMode, vk::Filter filter, vk::SamplerMipmapMode mipMapMode, bool compareOp)
	{
		vk::SamplerCreateInfo samplerInfo({},
			filter, filter,
			mipMapMode,
			addressMode, addressMode, addressMode,
			0.0f,
			VK_TRUE, 1.0f,			// TODO: add user control of max antriospy
			compareOp ? VK_TRUE : VK_FALSE, vk::CompareOp::eLessOrEqual,
			0.0f, VK_LOD_CLAMP_NONE,			// maxLod should equal the mip-map count?
			vk::BorderColor::eFloatTransparentBlack,
			VK_FALSE);

		VK_CHECK_RESULT(device.createSampler(&samplerInfo, nullptr, &sampler));
	}
}
