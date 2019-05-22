#include "RenderUtil.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/RenderPass.h"
#include "Vulkan/Descriptors.h"
#include "Vulkan/Shader.h"
#include "Vulkan/Queue.h"
#include "Vulkan/Image.h"
#include "Vulkan/Sampler.h"
#include "Rendering/StockModels.h"
#include "OEMaths/OEMaths_transform.h"

namespace OmegaEngine
{
	namespace RenderUtil
	{

		VulkanAPI::Texture generate_bdrf(vk::Device device, vk::PhysicalDevice& gpu, VulkanAPI::Queue& graph_queue)
		{
			const uint32_t lut_dim = 512;
			const vk::Format lut_format = vk::Format::eR16G16Sfloat;
			vk::ClearColorValue clear_value;

			VulkanAPI::Texture texture(device, gpu, graph_queue);
			texture.create_empty_image(lut_format, lut_dim, lut_dim, 1, vk::ImageUsageFlagBits::eColorAttachment);

			// setup renderpass
			VulkanAPI::RenderPass renderpass(device);
			renderpass.addAttachment(vk::ImageLayout::eColorAttachmentOptimal, lut_format);
			renderpass.prepareRenderPass();

			// and the frame buffer
			renderpass.prepareFramebuffer(texture.get_image_view(), lut_dim, lut_dim);

			// prepare the shader
			VulkanAPI::Shader shader;
			shader.add(device, "lut_bdrf-vert.spv", VulkanAPI::StageType::Vertex, "lut_bdrf-frag.spv", VulkanAPI::StageType::Fragment);

			// and the pipeline
			VulkanAPI::Pipeline pipeline;
			pipeline.add_shader(shader);
			pipeline.set_renderpass(renderpass);
			pipeline.add_colour_attachment(VK_FALSE, renderpass);
			pipeline.add_empty_layout();
			pipeline.create(device, VulkanAPI::PipelineType::Graphics);

			// and finally the command buffers
			VulkanAPI::CommandBuffer cmd_buffer(device, graph_queue.get_index());

			vk::RenderPassBeginInfo begin_info = renderpass.get_begin_info(clear_value);
			cmd_buffer.begin_renderpass(begin_info);
			cmd_buffer.bind_pipeline(pipeline);
			cmd_buffer.draw_quad();

			// push straight to the graphics queue
			graph_queue.flush_cmd_buffer(cmd_buffer.get());

			return texture;
		}
		
		VulkanAPI::Texture generate_irradiance_map(vk::Device device, vk::PhysicalDevice& gpu, VulkanAPI::Queue& graph_queue)
		{
			const uint32_t irradiance_dim = 64;
			const uint8_t mip_levels = 5;
			vk::ClearColorValue clear_value;
			
			// cube texture
			VulkanAPI::Texture cube_tex(device, gpu, graph_queue);
			cube_tex.create_empty_image(vk::Format::eR32G32B32A32Sfloat, irradiance_dim, irradiance_dim, mip_levels, vk::ImageUsageFlagBits::eColorAttachment);

			// offscreen texture
			VulkanAPI::Texture offscreen_tex(device, gpu, graph_queue);
			offscreen_tex.create_empty_image(vk::Format::eR32G32B32A32Sfloat, irradiance_dim, irradiance_dim, mip_levels, vk::ImageUsageFlagBits::eColorAttachment);

			// renderpass and framebuffer
			VulkanAPI::RenderPass renderpass(device);
			renderpass.addAttachment(vk::ImageLayout::eShaderReadOnlyOptimal, vk::Format::eR32G32B32A32Sfloat);
			renderpass.prepareRenderPass();
			renderpass.prepareFramebuffer(offscreen_tex.get_image_view(), irradiance_dim, irradiance_dim);

			// prepare the shader
			VulkanAPI::DescriptorLayout descr_layout;
			VulkanAPI::Shader shader;
			VulkanAPI::PipelineLayout pl_layout;
			shader.add(device, "env/irradiance_map-vert.spv", VulkanAPI::StageType::Vertex, "env/irradiance-frag.spv", VulkanAPI::StageType::Fragment);
			
			// use reflection to fill in the pipeline layout and descriptors
			shader.pipeline_layout_reflect(pl_layout);
			VulkanAPI::ImageLayoutBuffer sampler_layout;
			shader.descriptor_image_reflect(descr_layout, sampler_layout);

			// descriptor sets
			VulkanAPI::DescriptorSet descr_set(device, descr_layout);
			VulkanAPI::Sampler linear_sampler(device, VulkanAPI::SamplerType::LinearClamp);
			descr_set.write_set(sampler_layout[0][0].set, sampler_layout[0][0].binding, vk::DescriptorType::eSampler, linear_sampler.get_sampler(), cube_tex.get_image_view(), vk::ImageLayout::eShaderReadOnlyOptimal);

			// pipeline
			VulkanAPI::Pipeline pipeline;
			pipeline.add_shader(shader);
			pipeline.set_renderpass(renderpass);
			pipeline.add_colour_attachment(VK_FALSE, renderpass);
			pipeline.add_layout(pl_layout.get());
			pipeline.create(device, VulkanAPI::PipelineType::Graphics);

			// use the stock cube mesh
			RenderUtil::CubeModel cube_model;

			// record command buffer
			VulkanAPI::CommandBuffer cmd_buffer(device, graph_queue.get_index());
			vk::RenderPassBeginInfo begin_info = renderpass.get_begin_info(clear_value);
			
			// transition cube texture for transfer
			cube_tex.get_image().transition(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, cmd_buffer.get());

			// record command buffer for each mip and their layers
			for (uint8_t mip = 0; mip < mip_levels; ++mip) {
				for (uint8_t layer = 0; layer < 6; ++layer) {

					// get dimensions for this mip level
					float mip_dim = static_cast<float>(irradiance_dim * std::pow(0.5, mip));
					vk::Viewport view_port(mip_dim, mip_dim, 0.0f, 1.0f);
					
					cmd_buffer.begin_renderpass(begin_info, view_port);
					cmd_buffer.bind_pipeline(pipeline);
					cmd_buffer.bind_descriptors(pl_layout, descr_set, VulkanAPI::PipelineType::Graphics);
					//cmd_buffer.bind_vertex_buffer(cube_model.get_vertex_buffer());
					//cmd_buffer.bind_index_buffer(cube_model.get_index_buffer());

					// calculate view for each cube side
					FilterPushConstant push_block;
					push_block.mvp = OEMaths::perspective(static_cast<float>(M_PI) / 2.0f, 1.0f, 0.1f, 512.0f) * cubeView[layer];
					cmd_buffer.bind_push_block(pl_layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, sizeof(FilterPushConstant), &push_block);
					
					// draw cube into offscreen framebuffer
					cmd_buffer.draw_indexed(cube_model.get_index_count());
					cmd_buffer.end_pass();

					// copy the offscreen buffer to the current layer
					vk::ImageSubresourceLayers src_resource(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
					vk::Offset3D src_offset(0, 0, 0);
					vk::ImageSubresourceLayers dst_resource(vk::ImageAspectFlagBits::eColor, mip, layer, 1);
					vk::Offset3D dst_offset(0, 0, 0);
					vk::Extent3D extent(static_cast<uint32_t>(mip_dim), static_cast<uint32_t>(mip_dim), 1);
					vk::ImageCopy image_copy(src_resource, src_offset, dst_resource, dst_offset, extent);

					offscreen_tex.get_image().transition(vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal, cmd_buffer.get());
					cmd_buffer.get().copyImage(offscreen_tex.get_image().get(), vk::ImageLayout::eTransferSrcOptimal, cube_tex.get_image().get(), vk::ImageLayout::eTransferDstOptimal, 1, &image_copy);
					
					// transition the offscreen image back to colour attachment ready for the next image
					offscreen_tex.get_image().transition(vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eColorAttachmentOptimal, cmd_buffer.get());
				}
			}
			cube_tex.get_image().transition(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eUndefined, cmd_buffer.get());

			graph_queue.flush_cmd_buffer(cmd_buffer.get());
		}
	}
}