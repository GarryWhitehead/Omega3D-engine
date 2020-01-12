#include "Utility.h"

#include "utility/Logger.h"

namespace VulkanAPI
{

namespace VkUtil
{

vk::Format findSupportedFormat(std::vector<vk::Format>& formats, vk::ImageTiling tiling,
                               vk::FormatFeatureFlags formatFeature, vk::PhysicalDevice& gpu)
{
	vk::Format outputFormat;

	for (auto format : formats)
	{
		vk::FormatProperties properties = gpu.getFormatProperties(format);

		if (tiling == vk::ImageTiling::eLinear && formatFeature == (properties.linearTilingFeatures & formatFeature))
		{
			outputFormat = format;
			break;
		}
		else if (tiling == vk::ImageTiling::eOptimal &&
		         formatFeature == (properties.optimalTilingFeatures & formatFeature))
		{
			outputFormat = format;
			break;
		}
		else
		{
			// terminal error so throw
			throw std::runtime_error("Error! Unable to find supported vulkan format");
		}
	}
	return outputFormat;
}

vk::Format getSupportedDepthFormat(vk::PhysicalDevice& gpu)
{
	// in order of preference - TODO: allow user to define whether stencil format is required or not
	std::vector<vk::Format> formats = { vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint,
		                                vk::Format::eD32Sfloat };

	return findSupportedFormat(formats, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment,
	                           gpu);
}

vk::Format imageFormatToVk(const OmegaEngine::ImageFormat imageFormat)
{
	vk::Format ret = vk::Format::eUndefined;
	switch (imageFormat)
	{
	case OmegaEngine::ImageFormat::Etc_RGBA_UnormBlock:
		ret = vk::Format::eEtc2R8G8B8A8UnormBlock;
		break;
	case OmegaEngine::ImageFormat::Etc_RGBA_SrgbBlock:
		ret = vk::Format::eEtc2R8G8B8A8SrgbBlock;
		break;
	case OmegaEngine::ImageFormat::Etc_RGB_UnormBlock:
		ret = vk::Format::eEtc2R8G8B8UnormBlock;
		break;
	case OmegaEngine::ImageFormat::Etc_RGB_SrgbBlock:
		ret = vk::Format::eEtc2R8G8B8SrgbBlock;
		break;
	case OmegaEngine::ImageFormat::RGBA_Unorm:
		ret = vk::Format::eR8G8B8A8Unorm;
		break;
	case OmegaEngine::ImageFormat::RGB_Unorm:
		ret = vk::Format::eR8G8B8Unorm;
	default:
		LOGGER_ERROR("Unsupported image format requested.");
		break;
	}
	return ret;
}

vk::PrimitiveTopology topologyToVk(const OmegaEngine::Topology topology)
{
	vk::PrimitiveTopology ret;
	switch (topology)
	{
	case OmegaEngine::Topology::LineList:
		ret = vk::PrimitiveTopology::eLineList;
		break;
	case OmegaEngine::Topology::LineStrip:
		ret = vk::PrimitiveTopology::eLineStrip;
		break;
	case OmegaEngine::Topology::PointList:
		ret = vk::PrimitiveTopology::ePointList;
		break;
	case OmegaEngine::Topology::TriangleStrip:
		ret = vk::PrimitiveTopology::eTriangleStrip;
		break;
	case OmegaEngine::Topology::TrinagleList:
		ret = vk::PrimitiveTopology::eTriangleList;
		break;
	case OmegaEngine::Topology::TriangleFan:
		ret = vk::PrimitiveTopology::eTriangleFan;
		break;
	case OmegaEngine::Topology::Undefined:
		LOGGER_ERROR("You haven 't set the topolgy for this material!!");
		break;
	default:
		LOGGER_ERROR("Unsupported topology type requested");
	}
	return ret;
}

}    // namespace VkUtil
}    // namespace VulkanAPI
