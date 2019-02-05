#pragma once
#include "Vulkan/Common.h"

// forward decleration
namespace VulkanAPI
{
	class Queue;
	class Texture;
}

namespace OmegaEngine
{
	namespace RenderUtil
	{
		static VulkanAPI::Texture generate_bdrf(vk::Device device, VulkanAPI::Queue& graph_queue);
		static VulkanAPI::Texture generate_irradiance_map(vk::Device device, VulkanAPI::Queue& graph_queue);
	}
}