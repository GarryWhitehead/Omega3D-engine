#pragma once

namespace OmegaEngine
{

	enum class RenderTypes
	{
		Mesh,
		Skybox,
		Ocean,
		Terrain,
		Count
	};

	// abstract base class
	class RenderableBase
	{
	public:

		RenderableBase(RenderTypes t, uint32_t key) :
			type(t),
			priority_key(key)
		{
		}

		virtual ~RenderableBase() {}

		// abstract render call
		virtual render(VulkanAPI::CommandBuffer& cmd_buffer, 
						RenderPipeline render_pipeline, 
						ThreadPool& thread_pool, 
						uint32_t threads_per_group, uint32_t num_threads) = 0;

		RenderTypes get_type() const
		{
			return type;
		}

	protected:

		RenderTypes type;

		// tells the renderer when this should be rendered in the queue
		uint32_t priority_key = 0;
	};

}