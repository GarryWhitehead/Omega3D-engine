#include "RenderUtil.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/RenderPass.h"
#include "Vulkan/Shader.h"
#include "Vulkan/Queue.h"
#include "Vulkan/Image.h"

namespace OmegaEngine
{
	namespace RenderUtil
	{

		VulkanAPI::Texture generate_bdrf(vk::Device device, VulkanAPI::Queue& graph_queue)
		{
			const float lut_dim = 512;
			const vk::Format lut_format = vk::Format::eR16G16Sfloat;
			vk::ClearColorValue clear_value;

			VulkanAPI::Texture texture(VulkanAPI::TextureType::Normal);
			texture.create_empty_image(lut_format, lut_dim, lut_dim, 1, vk::ImageUsageFlagBits::eColorAttachment);

			// setup renderpass
			VulkanAPI::RenderPass renderpass(device);
			renderpass.addAttachment(vk::ImageLayout::eColorAttachmentOptimal, lut_format);
			renderpass.addReference(vk::ImageLayout::eColorAttachmentOptimal, 0);
			renderpass.prepareRenderPass();

			// and the frame buffer
			renderpass.prepareFramebuffer(texture.get_image_view(), lut_dim, lut_dim);

			// prepare the shader
			VulkanAPI::Shader shader;
			shader.add(device, "lut_bdrf-vert.spv", VulkanAPI::StageType::Vertex, "lut_bdrf-frag.spv", VulkanAPI::StageType::Fragment);

			// and the pipeline
			VulkanAPI::Pipeline pipeline(device, VulkanAPI::PipelineType::Graphics);
			pipeline.add_shader(shader);
			pipeline.set_renderpass(renderpass.get());
			pipeline.add_colour_attachment(VK_FALSE);
			pipeline.add_empty_layout();
			pipeline.create();

			// and finally the command buffers
			VulkanAPI::CommandBuffer cmd_buffer(device);

			vk::RenderPassBeginInfo begin_info = renderpass.get_begin_info(clear_value);
			cmd_buffer.begin_renderpass(begin_info);
			cmd_buffer.bind_pipeline(pipeline);
			cmd_buffer.draw_quad();

			// push straight to the graphics queue
			graph_queue.submit_cmd_buffer(cmd_buffer.get());

			return texture;
		}
		
		VulkanAPI::Texture generate_irradiance_map(vk::Device device, VulkanAPI::Queue& graph_queue)
		{
			const uint32_t irradiance_dim = 64;
			const uint8_t mip_levels = 5;
			vk::ClearColorValue clear_value;
			
			// cube texture
			VulkanAPI::Texture cube_tex(VulkanAPI::TextureType::CubeArray);
			cube_tex.create_empty_image(vk::Format::eR32G32B32A32Sfloat, irradiance_dim, irradiance_dim, mip_levels, vk::ImageUsageFlagBits::eColorAttachment);

			// offscreen texture
			VulkanAPI::Texture offscreen_tex(VulkanAPI::TextureType::Normal);
			offscreen_tex.create_empty_image(vk::Format::eR32G32B32A32Sfloat, irradiance_dim, irradiance_dim, mip_levels, vk::ImageUsageFlagBits::eColorAttachment);

			// renderpass and framebuffer
			VulkanAPI::RenderPass renderpass(device);
			renderpass.addAttachment(vk::ImageLayout::eColorAttachmentOptimal, vk::Format::eR32G32B32A32Sfloat);
			renderpass.addReference(vk::ImageLayout::eColorAttachmentOptimal, 0);
			renderpass.prepareRenderPass();
			renderpass.prepareFramebuffer(offscreen_tex.get_image_view(), irradiance_dim, irradiance_dim);

			// prepare the shader
			VulkanAPI::Shader shader;
			VulkanAPI::PipelineLayout pl_layout;
			shader.add(device, "env/irradiance_map-vert.spv", VulkanAPI::StageType::Vertex, "env/irradiance-frag.spv", VulkanAPI::StageType::Fragment);
			shader.reflection(VulkanAPI::StageType::Vertex, descr_layout, pl_layout, pipeline);

			// pipeline
			VulkanAPI::Pipeline pipeline(device, VulkanAPI::PipelineType::Graphics);
			pipeline.add_shader(shader);
			pipeline.set_renderpass(renderpass.get());
			pipeline.add_colour_attachment(VK_FALSE);
			pipeline.add_layout(pl_layout.get());
			pipeline.create();

			// record command buffer
			VulkanAPI::CommandBuffer cmd_buffer(device);
			vk::RenderPassBeginInfo begin_info = renderpass.get_begin_info(clear_value);
			
			// transition cube texture for transfer
			cube_tex.get_image().transition(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 1, cmd_buffer.get(), graph_queue, cmd_buffer.get_pool());

			// record command buffer for each mip and their layers
			for (uint8_t mip = 0; mip < mip_levels; ++mip) {
				for (uint8_t layer = 0; layer < 6; ++layer) {

					// get dimensions for this mip level
					float mip_dim = static_cast<float>(irradiance_dim * std::pow(0.5, mip));
					vk::Viewport view_port(static_cast<uint32_t>(mip_dim), static_cast<uint32_t>(mip_dim), 0.0f, 1.0f);
					
					cmd_buffer.begin_renderpass(begin_info, view_port);
					cmd_buffer.bind_pipeline(pipeline);
					cmd_buffer.bind_descriptors(pl_layout, descr_set, VulkanAPI::PipelineType::Graphics);
					cmd_buffer.bind_vertex_buffer();
					cmd_buffer.bind_index_buffer();

					cmd_buffer.bind_push_block(pl_layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, sizeof(push_block), &push);
					
					// draw cube into offscreen framebuffer
					cmd_buffer.draw_indexed(, 1, 0, 0, 0);
					cmd_buffer.end_pass();

					// copy the offscreen buffer to the current layer
					vk::ImageSubresourceLayers src_resource(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
					vk::Offset3D src_offset(0, 0, 0);
					vk::ImageSubresourceLayers dst_resource(vk::ImageAspectFlagBits::eColor, mip, layer, 1);
					vk::Offset3D dst_offset(0, 0, 0);
					vk::Extent3D extent(static_cast<uint32_t>(mip_dim), static_cast<uint32_t>(mip_dim), 1);
					vk::ImageCopy image_copy(src_resource, src_offset, dst_resource, dst_offset, extent);

					offscreen_tex.get_image().transition(vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal, 1, cmd_buffer.get(), graph_queue, cmd_buffer.get_pool());
					cmd_buffer.get().copyImage(offscreen_tex.get_image(), vk::ImageLayout::eTransferSrcOptimal, cube_tex.get_image(), vk::ImageLayout::eTransferDstOptimal, 1, &image_copy);
					
					// transition the offscreen image back to colour attachment ready for the next image
					offscreen_tex.get_image().transition(vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eColorAttachmentOptimal, 1, cmd_buffer.get(), graph_queue, cmd_buffer.get_pool());
				}
			}
			cube_tex.get_image().transition(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eUndefined, 1, cmd_buffer.get(), graph_queue, cmd_buffer.get_pool());

			graph_queue.submit_cmd_buffer(cmd_buffer.get());
		}
	}
}