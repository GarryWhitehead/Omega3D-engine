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
#include "Vulkan/Common.h"
#include "Utility/logger.h"
#include "Threading/ThreadPool.h"

namespace OmegaEngine
{

	RenderInterface::RenderInterface(VulkanAPI::Device device, const uint32_t win_width, const uint32_t win_height)
	{
		// initiliase the graphical backend - we are solely using Vulkan 
		vk_interface = std::make_unique<VulkanAPI::Interface>(device, win_width, win_height);

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

	void RenderInterface::render_stage(double interpolation, const RenderStage stage)
	{
		uint32_t num_threads = Util::HardWareConcurrency();
		ThreadPool thread_pool(num_threads);

		uint32_t threads_per_group = VULKAN_THREADED_GROUP_SIZE / num_threads;

		// begin the renderer and get the cmd_buffer
		VulkanAPI::CommandBuffer cmd_buffer = renderer->begin();

		for (auto& renderable : renderables[stage]) {

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
		// render the stages in the correct order - i.e deferred rendering to offscreen g-buffers must come before post-render effects
		for (uint8_t stage = 0; stage < (uint8_t)RenderStage::Count; ++stage) {
		
			if (stage == (uint8_t)RenderStage::Deferred && render_config.general.renderer != RendererType::Deferred) {
				continue;
			}
			render_stage(interpolation, static_cast<RenderStage>(stage));
		}
	}

}
