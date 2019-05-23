#pragma once
#include "Vulkan/Common.h"
#include "Vulkan/CommandBuffer.h"

// forward declerations

namespace OmegaEngine
{

	class PostProcessInterface
	{

	public:

		PostProcessInterface(vk::Device dev);
		~PostProcessInterface();

		void render();

	private:

		vk::Device device;

		VulkanAPI::CommandBuffer cmdBuffer;
	};

}

