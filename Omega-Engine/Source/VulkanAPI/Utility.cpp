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

vk::Format getVkFormatFromType(std::string type, uint32_t width)
{
    // TODO: add other base types and widths
    vk::Format format = vk::Format::eUndefined;

    // floats
    if (width == 32)
    {
        if (type == "float")
        {
            format = vk::Format::eR32Sfloat;
        }
        else if (type == "vec2")
        {
            format = vk::Format::eR32G32Sfloat;
        }
        else if (type == "vec3")
        {
            format = vk::Format::eR32G32B32Sfloat;
        }
        else if (type == "vec4")
        {
            format = vk::Format::eR32G32B32A32Sfloat;
        }
        // signed integers
        else if (type == "int")
        {
            format = vk::Format::eR32Sint;
        }
        else if (type == "ivec2")
        {
            format = vk::Format::eR32G32Sint;
        }
        else if (type == "ivec3")
        {
            format = vk::Format::eR32G32B32Sint;
        }
        else if (type == "ivec4")
        {
            format = vk::Format::eR32G32B32A32Sint;
        }
        else
        {
            LOGGER_ERROR("Unsupported Vulkan format type specified: %s", type.c_str());
        }
    }

    return format;
}

uint32_t getStrideFromType(std::string type)
{
    // TODO: add other base types and widths
    uint32_t size = 0;

    // floats
    if (type == "float" || type == "int")
    {
        size = 4;
    }
    else if (type == "vec2" || type == "ivec2")
    {
        size = 8;
    }
    else if (type == "vec3" || type == "ivec3")
    {
        size = 12;
    }
    else if (type == "vec4" || type == "ivec4")
    {
        size = 16;
    }
    else
    {
        LOGGER_ERROR(
            "Unsupported type specified: %s. Unable to determine stride size.", type.c_str());
    }

    return size;
}

bool isDepth(const vk::Format format)
{
    std::vector<vk::Format> depthFormats = {vk::Format::eD16Unorm,
                                            vk::Format::eX8D24UnormPack32,
                                            vk::Format::eD32Sfloat,
                                            vk::Format::eD16UnormS8Uint,
                                            vk::Format::eD24UnormS8Uint,
                                            vk::Format::eD32SfloatS8Uint};
    return std::find(depthFormats.begin(), depthFormats.end(), format) != std::end(depthFormats);
}

bool isStencil(const vk::Format format)
{
    std::vector<vk::Format> stencilFormats = {vk::Format::eS8Uint,
                                              vk::Format::eD16UnormS8Uint,
                                              vk::Format::eD24UnormS8Uint,
                                              vk::Format::eD32SfloatS8Uint};
    return std::find(stencilFormats.begin(), stencilFormats.end(), format) !=
        std::end(stencilFormats);
}

}    // namespace VkUtil
}    // namespace VulkanAPI
