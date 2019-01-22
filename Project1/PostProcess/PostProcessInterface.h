#pragma once
#include "Vulkan/Common.h"

// forward declerations
namespace VulkanAPI
{
	class CommandBuffer;
}

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

		VulkanAPI::CommandBuffer& cmd_buffer;
	};

}

