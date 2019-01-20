#include "RenderUtil.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/RenderPass.h"
#include "Vulkan/Shader.h"
#include "Vulkan/Queue.h"

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
	
	}
}