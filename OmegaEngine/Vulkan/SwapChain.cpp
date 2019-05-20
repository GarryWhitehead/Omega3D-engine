#include "SwapChain.h"
#include "Vulkan/Device.h"
#include "Utility/logger.h"
#include <algorithm>

namespace VulkanAPI
{
	Swapchain::Swapchain()
	{
	}

	void Swapchain::create(vk::Device dev, 
							vk::PhysicalDevice& phys_dev, 
							vk::SurfaceKHR& surface, 
							uint32_t graph_index, uint32_t present_index, 
							uint32_t screen_width, uint32_t screen_height)
	{
		this->device = dev;
		this->gpu = phys_dev;

		// Get the basic surface properties of the physical device
		uint32_t surfaceCount = 0;

		vk::SurfaceCapabilitiesKHR capabilities = gpu.getSurfaceCapabilitiesKHR(surface);
		std::vector<vk::SurfaceFormatKHR> formats = gpu.getSurfaceFormatsKHR(surface);
		std::vector<vk::PresentModeKHR> present_modes = gpu.getSurfacePresentModesKHR(surface);

		// make sure that we have suitable swap chain extensions available before continuing
		if (formats.empty() || present_modes.empty()) 
		{
			LOGGER_ERROR("Critcal error! Unable to locate suitable swap chains on device.");
		}

		// Next step is to determine the surface format. Ideally undefined format is preffered so we can set our own, otherwise
		// we will go with one that suits our colour needs - i.e. 8bitBGRA and SRGB.
		vk::SurfaceFormatKHR req_surf_format;

		if ((formats.size() > 0) && (formats[0].format == vk::Format::eUndefined))
		{
			req_surf_format.format = vk::Format::eB8G8R8A8Unorm;
			req_surf_format.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
		}
		else
		{
			for (auto& format : formats)
			{
				if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
				{
					req_surf_format = format;
					break;
				}
			}
		}
		format = req_surf_format;

		// And then the presentation format - the preferred format is triple buffering
		vk::PresentModeKHR req_mode;
		for (auto& mode : present_modes)
		{
			if (mode == vk::PresentModeKHR::eMailbox)													// our preferred triple buffering mode
			{
				req_mode = mode;
				break;
			}
			else if (mode == vk::PresentModeKHR::eImmediate)
			{
				req_mode = mode;																	// immediate mode is only supported on some drivers.
				break;
			}
		}
		
		// Finally set the resoultion of the swap chain buffers
		// First of check if we can manually set the dimension - some GPUs allow this by setting the max as the size of uint32
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			extent = capabilities.currentExtent;	// go with the automatic settings
		} 
		else
		{
			extent = { screen_width, screen_height };
			extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent.width));
			extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent.height));
		}

		// Get the number of possible images we can send to the queue
		uint32_t imageCount = capabilities.minImageCount + 1;								 // adding one as we would like to implement triple buffering
		if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) 
		{
			imageCount = capabilities.maxImageCount;
		}

		// if the graphics and presentation aren't the same then use concurrent sharing mode
		std::vector<uint32_t> queue_family_indicies;
		vk::SharingMode sharing_mode = vk::SharingMode::eExclusive;

		if (graph_index != present_index)
		{
			sharing_mode = vk::SharingMode::eConcurrent;
			queue_family_indicies.push_back(graph_index);
			queue_family_indicies.push_back(present_index);
		}

		vk::SwapchainCreateInfoKHR createInfo({},
			surface,
			imageCount,
			req_surf_format.format,
			req_surf_format.colorSpace,
			extent,
			1, vk::ImageUsageFlagBits::eColorAttachment,
			sharing_mode,
			0, nullptr,
			capabilities.currentTransform,
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			req_mode, VK_TRUE, {});
		
		// And finally, create the swap chain
		VK_CHECK_RESULT(device.createSwapchainKHR(&createInfo, nullptr, &swapchain));

		// Get the image loactions created when creating the swap chains
		std::vector<vk::Image> images = device.getSwapchainImagesKHR(swapchain);

		for (int c = 0; c < images.size(); ++c)
		{
			ImageView imageView; 
		    imageView.create(device, images[c], req_surf_format.format, vk::ImageAspectFlagBits::eColor, vk::ImageViewType::e2D);
			image_views.emplace_back(std::move(imageView));
		}

		prepare_swapchain_pass();
	}

	Swapchain::~Swapchain()
	{
		/*for (int c = 0; c < image_views.size(); ++c)
		{
			device.destroyImageView(image_views[c].get_imageView(), nullptr);
		}

		device.destroySwapchainKHR(swapchain, nullptr);*/
	}

	void Swapchain::begin_frame(vk::Semaphore& semaphore)
	{
		device.acquireNextImageKHR(swapchain, std::numeric_limits<uint64_t>::max(), semaphore, {}, &image_index);
	}

	void Swapchain::submit_frame(vk::Semaphore& present_semaphore, vk::Queue& present_queue)
	{
		vk::PresentInfoKHR present_info(
			1, &present_semaphore,
			1, &swapchain,
			&image_index,
			nullptr);

		VK_CHECK_RESULT(present_queue.presentKHR(&present_info));
		present_queue.waitIdle();
	}

	void Swapchain::prepare_swapchain_pass()
	{
		// depth image
		vk::Format depth_format = VulkanAPI::Device::get_depth_format(gpu);
		
		depth_texture = std::make_unique<Texture>(device, gpu);
		depth_texture->create_empty_image(depth_format, extent.width, extent.height, 1, vk::ImageUsageFlagBits::eDepthStencilAttachment);

		renderpass = std::make_unique<RenderPass>(device);
		renderpass->addAttachment(vk::ImageLayout::ePresentSrcKHR, format.format);
		renderpass->addAttachment(vk::ImageLayout::eDepthStencilAttachmentOptimal, depth_format);
		renderpass->prepareRenderPass();

		// create presentation renderpass/framebuffer for each swap chain image
		for (uint32_t i = 0; i < image_views.size(); ++i) {

			// create a frame buffer for each swapchain image
			std::vector<vk::ImageView> views{ image_views[i].get_imageView(), depth_texture->get_image_view() };
			renderpass->prepareFramebuffer(static_cast<uint32_t>(views.size()), views.data(), extent.width, extent.height);
		}
	}

}