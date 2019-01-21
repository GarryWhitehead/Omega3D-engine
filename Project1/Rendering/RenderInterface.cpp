#include "RenderInterface.h"
#include "RenderableTypes/RenderableBase.h"
#include "RenderableTypes/Mesh.h"
#include "Rendering/Renderer.h"
#include "ComponentInterface/ComponentInterface.h"
#include "DataTypes/Object.h"
#include "Managers/TransformManager.h"
#include "Managers/MeshManager.h"
#include "Managers/MaterialManager.h"
#include "Managers/TextureManager.h"
#include "Vulkan/MemoryAllocator.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Queue.h"
#include "PostProcess/PostProcessInterface.h"
#include "Utility/logger.h"
#include "Threading/ThreadPool.h"

namespace OmegaEngine
{

	RenderInterface::RenderInterface(VulkanAPI::Device device, const uint32_t win_width, const uint32_t win_height)
	{
		// load the render config file if it exsists
		
		// initiliase the graphical backend - we are solely using Vulkan 
		vk_interface = std::make_unique<VulkanAPI::Interface>(device, win_width, win_height);

		// initialise pre and post renderers
		renderer = std::make_unique<Renderer>(device, render_config.general.renderer);
		postprocess_interface = std::make_unique<PostProcessInterface>(device);

		// initlaise all shaders and pipelines that will be used which is dependent on the number of renderable types
		for (uint16_t r_type = 0; r_type < (uint16_t)RenderTypes::Count; ++r_type) {
			this->add_shader((RenderTypes)r_type);
		}
	}


	RenderInterface::~RenderInterface()
	{
	}


	void RenderInterface::add_shader(RenderTypes type)
	{
		VulkanAPI::Shader shader;
		switch (type) {
		case OmegaEngine::RenderTypes::Mesh:
			render_pipelines[(int)RenderTypes::Mesh] = RenderableMesh::create_mesh_pipeline(vk_interface->get_device());
			break;
		case OmegaEngine::RenderTypes::Skybox:
			shader.add(vk_interface->get_device(), "env/skybox.vert", VulkanAPI::StageType::Vertex, "env/skybox.frag", VulkanAPI::StageType::Fragment);
			break;
		default:
			LOGGER_INFO("Unsupported render type found whilst initilaising shaders.");
		}

		render_pipelines[(int)type].shader = shader;
	}

	void RenderInterface::submit_to_graphics_queue(const RenderStage stage)
	{
		VulkanAPI::Queue graph_queue = vk_interface->get_graph_queue();
		graph_queue.submit_cmd_buffer(cmd_buffers[(int)stage].get(), semaphores[s)
	}

	void RenderInterface::render_components(VulkanAPI::CommandBuffer& cmd_buffer, double interpolation)
	{
		uint32_t num_threads = Util::HardWareConcurrency();
		ThreadPool thread_pool(num_threads);

		uint32_t threads_per_group = VULKAN_THREADED_GROUP_SIZE / num_threads;

		for (auto& renderable : renderables) {

			// Note: this is in a specific order in whcih renderable should be drawn
			switch (renderable->get_type()) {
			case RenderTypes::Skybox:
				break;
			case RenderTypes::Mesh:
				static_cast<RenderableMesh*>(renderable.get())->render(cmd_buffer, render_pipelines[(int)RenderTypes::Mesh], thread_pool, threads_per_group);
				break;
			default:
				LOGGER_INFO("Error whilst rendering. Unsupported renderable detected.");
			}
		}

		// check that all threads are finshed before executing the cmd buffers
		thread_pool.wait_for_all();

		cmd_buffer.secondary_execute_commands();
		cmd_buffer.end();
	}

	void RenderInterface::render(double interpolation)
	{
		// Note: only deferred supported at the moment but this will change once Forward rendering is added
		// first stage of the deferred render pipeline is to generate the g-buffers by drawing the components into the offscreen frame-buffers
		VulkanAPI::CommandBuffer cmd_buffer = cmd_buffers[(int)RenderStage::Deferred];
		cmd_buffer.init(device);
		cmd_buffer.create_primary();

		// begin the renderpass 
		vk::RenderPassBeginInfo begin_info = renderer->get_renderpass()->get_begin_info();
		cmd_buffer.begin_renderpass(begin_info);

		// render the components
		render_components(cmd_buffer, interpolation);
		
		// and render the deffered pass - lights and IBL
		renderer->render(cmd_buffer);
		
		// post-processing is done in a separate forward pass using the offscreen buffer filled by the deferred pass
		VulkanAPI::CommandBuffer cmd_buffer = cmd_buffers[(int)RenderStage::PostProcess];
		cmd_buffer.init(device);
		cmd_buffer.create_primary();

		vk::RenderPassBeginInfo begin_info = renderer->get_renderpass()->get_begin_info();
		cmd_buffer.begin_renderpass(begin_info);

		postprocess_interface->render(cmd_buffer);

		// now submit the command buffers to the graphic queue
		submit_to_graphics_queue();
	}

}
