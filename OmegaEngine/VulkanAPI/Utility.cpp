#include "Utility.h"

#include "utility/Logger.h"

namespace VulkanAPI
{

namespace Util
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

	return findSupportedFormat(formats, vk::ImageTiling::eOptimal,
	                                       vk::FormatFeatureFlagBits::eDepthStencilAttachment, gpu);
}

}    // namespace Util
}    // namespace VulkanAPI
