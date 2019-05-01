#include "Skybox.h"
#include "Vulkan/Shader.h"
#include "Vulkan/VkTextureManager.h"
#include "Vulkan/CommandBuffer.h"
#include "Rendering/Renderers/RendererBase.h"
#include "Rendering/RenderInterface.h"
#include "Utility/logger.h"

namespace OmegaEngine
{

	RenderableSkybox::RenderableSkybox() :
		RenderableBase(RenderTypes::Skybox)
	{
		// fill out the data which will be used for rendering
		instance_data = new SkyboxInstance;
		SkyboxInstance* skybox_instance = reinterpret_cast<SkyboxInstance*>(instance_data);
	}


	RenderableSkybox::~RenderableSkybox()
	{
	}

	void RenderableSkybox::create_skybox_pipeline(vk::Device& device,
		std::unique_ptr<RendererBase>& renderer,
		std::unique_ptr<VulkanAPI::BufferManager>& buffer_manager,
		std::unique_ptr<VulkanAPI::VkTextureManager>& texture_manager,
		std::unique_ptr<RenderInterface::ProgramState>& state)
	{
		// load shaders
		if (!state->shader.add(device, "env/Skybox/skybox-vert.spv", VulkanAPI::StageType::Vertex, "env/Skybox/skybox-frag.spv", VulkanAPI::StageType::Fragment)) {
			LOGGER_ERROR("Unable to create skybox shaders.");
		}

		// get pipeline layout and vertedx attributes by reflection of shader
		state->shader.descriptor_image_reflect(state->descr_layout, state->image_layout);
		state->shader.descriptor_buffer_reflect(state->descr_layout, state->buffer_layout);
		state->descr_layout.create(device);

		// we only want to init the uniform buffer sets, the material image samplers will be created by the materials themselves
		for (auto& buffer : state->buffer_layout) {
			state->descr_set.init(device, state->descr_layout.get_layout(buffer.set), state->descr_layout.get_pool(), buffer.set);
		}

		// sort out the descriptor sets - buffers
		for (auto& layout : state->buffer_layout) {

			// the shader must use these identifying names for uniform buffers -
			if (layout.name == "CameraUbo") {
				buffer_manager->enqueueDescrUpdate("Camera", &state->descr_set, layout.set, layout.binding, layout.type);
			}
		}

		// sort out the descriptor sets - images
		for (auto& layout : state->image_layout) {

			for (auto& image : layout.second)
			{
				// the shader must use these identifying names for uniform buffers -
				if (image.name == "SkyboxSampler") {
					texture_manager->enqueueDescrUpdate("Skybox", &state->descr_set, &image.sampler, image.set, image.binding);
				}
			}
		}

		state->shader.pipeline_layout_reflect(state->pl_layout);
		state->pl_layout.create(device, state->descr_layout.get_layout());

		// create the graphics pipeline
		state->shader.pipeline_reflection(state->pipeline);

		state->pipeline.set_depth_state(VK_TRUE, VK_FALSE);
		state->pipeline.set_raster_cull_mode(vk::CullModeFlagBits::eBack);
		state->pipeline.set_raster_front_face(vk::FrontFace::eClockwise);
		state->pipeline.set_topology(vk::PrimitiveTopology::eTriangleList);
		state->pipeline.add_colour_attachment(VK_FALSE, renderer->get_first_pass());
		state->pipeline.create(device, renderer->get_first_pass(), state->shader, state->pl_layout, VulkanAPI::PipelineType::Graphics);
	}

	void RenderableSkybox::render(VulkanAPI::SecondaryCommandBuffer& cmd_buffer, 
								void* instance,
								RenderInterface* render_interface)
	{
		SkyboxInstance* instance_data = (SkyboxInstance*)instance;
		RenderInterface::ProgramState* pipeline = render_interface->get_render_pipeline(RenderTypes::Skybox).get();

		cmd_buffer.set_viewport();
		cmd_buffer.set_scissor();
		cmd_buffer.bind_pipeline(pipeline->pipeline);
		cmd_buffer.bind_descriptors(pipeline->pl_layout, pipeline->descr_set, VulkanAPI::PipelineType::Graphics);
		cmd_buffer.bind_push_block(pipeline->pl_layout, vk::ShaderStageFlagBits::eFragment, sizeof(float), &instance_data->blur_factor);

		vk::DeviceSize offset = { instance_data->vertex_buffer.offset };
		cmd_buffer.bind_vertex_buffer(instance_data->vertex_buffer.buffer, offset);
		cmd_buffer.bind_index_buffer(instance_data->index_buffer.buffer, instance_data->index_buffer.offset);
		cmd_buffer.draw_indexed(instance_data->index_count);
	}
}
