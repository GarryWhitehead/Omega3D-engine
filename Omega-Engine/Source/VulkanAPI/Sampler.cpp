#include "Sampler.h"

#include "VulkanAPI/VkContext.h"

#include "utility/Logger.h"

namespace VulkanAPI
{

Sampler::Sampler(SamplerType type)
    : type(type)
{
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

SamplerType Sampler::getDefaultSampler()
{
	return SamplerType::LinearClamp;
}

void Sampler::build(VkContext& context)
{
	switch (type)
	{
	case SamplerType::Clamp:
		createSampler(context, vk::SamplerAddressMode::eClampToEdge, vk::Filter::eNearest,
		              vk::SamplerMipmapMode::eNearest, false);
		break;
	case SamplerType::Wrap:
		createSampler(context, vk::SamplerAddressMode::eRepeat, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest,
		              false);
		break;
	case SamplerType::LinearClamp:
		createSampler(context, vk::SamplerAddressMode::eClampToEdge, vk::Filter::eLinear,
		              vk::SamplerMipmapMode::eNearest, false);
		break;
	case SamplerType::TriLinearClamp:
		createSampler(context, vk::SamplerAddressMode::eClampToEdge, vk::Filter::eLinear,
		              vk::SamplerMipmapMode::eLinear, false);
		break;
	case SamplerType::LinearWrap:
		createSampler(context, vk::SamplerAddressMode::eRepeat, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest,
		              false);
		break;
	case SamplerType::TrilinearWrap:
		createSampler(context, vk::SamplerAddressMode::eRepeat, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear,
		              false);
		break;
	}
}

void Sampler::createSampler(VkContext& context, vk::SamplerAddressMode addressMode, vk::Filter filter,
                            vk::SamplerMipmapMode mipMapMode, bool compareOp)
{
	vk::SamplerCreateInfo samplerInfo({}, filter, filter, mipMapMode, addressMode, addressMode, addressMode, 0.0f,
	                                  VK_TRUE,
	                                  1.0f,    // TODO: add user control of max antriospy
	                                  compareOp ? VK_TRUE : VK_FALSE, vk::CompareOp::eLessOrEqual, 0.0f,
	                                  VK_LOD_CLAMP_NONE,    // maxLod should equal the mip-map count?
	                                  vk::BorderColor::eFloatTransparentBlack, VK_FALSE);

	VK_CHECK_RESULT(context.getDevice().createSampler(&samplerInfo, nullptr, &sampler));
}
}    // namespace VulkanAPI
