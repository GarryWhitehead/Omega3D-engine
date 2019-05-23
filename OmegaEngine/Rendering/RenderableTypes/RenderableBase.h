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
		ShadowStatic,
		ShadowDynamic,
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
			if(instanceData != nullptr) 
			{
				delete instanceData;
			} 
		}

		// abstract render call
		virtual void render(VulkanAPI::SecondaryCommandBuffer& cmdBuffer,
			void* renderableData)
		{
		}

		virtual void* getHandle() = 0;

		RenderTypes getRenderType() const
		{
			return type;
		}

		template <typename T>
		T* getInstanceData()
		{
			return reinterpret_cast<T*>(instanceData);
		}

		void* getInstanceData()
		{
			return instanceData;
		}

		SortKey& getSortKey()
		{
			return sort_key;
		}

		QueueType& getQueueType()
		{
			return queueType;
		}
		
	protected:

		RenderTypes type;

		// data used for rendering 
		void* instanceData = nullptr;

		// determines sort order of renderable
		SortKey sortKey;

		// how to render this renderable
		QueueType queueType;
	};

}