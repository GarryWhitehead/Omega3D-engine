#pragma once
#include <cstdint>
#include <memory>

// forward decleartions
namespace VulkanAPI
{
	class CommandBuffer;
}

namespace OmegaEngine
{
	// forward declerations
	class ThreadPool;
	class RenderPipeline;
	class ComponentInterface;

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
		virtual void render(VulkanAPI::CommandBuffer& cmd_buffer, 
							RenderPipeline& render_pipeline, 
							std::unique_ptr<ComponentInterface>& component_interface,
							RenderInterface* render_interface) = 0;

		RenderTypes get_type() const
		{
			return type;
		}

		uint32_t get_priority_key() const
		{
			return priority_key;
		}

	protected:

		RenderTypes type;

		// tells the renderer when this should be rendered in the queue
		uint32_t priority_key = 0;
	};

}