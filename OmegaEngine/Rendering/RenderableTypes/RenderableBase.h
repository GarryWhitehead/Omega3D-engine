#pragma once
#include <cstdint>
#include <memory>

#include "Rendering/RenderQueue.h"

// forward decleartions
namespace VulkanAPI
{
	class SecondaryCommandBuffer;
}

namespace OmegaEngine
{
	// forward declerations
	class RenderInterface;

	enum class RenderTypes
	{
		StaticMesh,
		SkinnedMesh,
		Skybox,
		Shadow,
		Count
	};

	// abstract base class
	class RenderableBase
	{
	public:

		RenderableBase(RenderTypes t) :
			type(t)
		{
		}

		virtual ~RenderableBase() 
		{
			if(instance_data != nullptr) {
				delete instance_data;
			} 
		}

		// abstract render call
		virtual void render(VulkanAPI::SecondaryCommandBuffer& cmd_buffer,
			void* renderable_data)
		{
		}

		virtual void* get_handle() = 0;

		RenderTypes get_type() const
		{
			return type;
		}

		void* get_instance_data()
		{
			return instance_data;
		}

		SortKey& get_sort_key()
		{
			return sort_key;
		}

		QueueType& get_queue_type()
		{
			return queue_type;
		}
		
	protected:

		RenderTypes type;

		// data used for rendering 
		void* instance_data = nullptr;

		// determines sort order of renderable
		SortKey sort_key;

		// how to render this renderable
		QueueType queue_type;
	};

}