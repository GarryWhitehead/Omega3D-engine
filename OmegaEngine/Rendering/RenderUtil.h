#pragma once
#include "Vulkan/Common.h"
#include "OEMaths/OEMaths.h"

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
		std::vector<OEMaths::mat4f> cubeView = {
			// POSITIVE_X
			OEMaths::mat4f::rotate(180.0f, OEMaths::vec3f(1.0f, 0.0f, 0.0f)) * OEMaths::mat4f::rotate(90.0f, OEMaths::vec3f(0.0f, 1.0f, 0.0f)),
			// NEGATIVE_X
			OEMaths::mat4f::rotate(180.0f, OEMaths::vec3f(1.0f, 0.0f, 0.0f)) * OEMaths::mat4f::rotate(-90.0f, OEMaths::vec3f(0.0f, 1.0f, 0.0f)),
			// POSITIVE_Y
			OEMaths::mat4f::rotate(90.0f, OEMaths::vec3f(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_Y
			OEMaths::mat4f::rotate(90.0f, OEMaths::vec3f(1.0f, 0.0f, 0.0f)),
			// POSITIVE_Z
			OEMaths::mat4f::rotate(180.0f, OEMaths::vec3f(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_Z
			OEMaths::mat4f::rotate(180.0f, OEMaths::vec3f(0.0f, 0.0f, 1.0f))
		};
	
		// push constant layout for pre-filtered cube
		struct FilterPushConstant
		{
			OEMaths::mat4f mvp;			// offset = 0
			float roughness;			// offset = 64
			uint32_t sampleCount;		// offset = 68
		};

		static VulkanAPI::Texture generate_bdrf(vk::Device device, vk::PhysicalDevice& gpu, VulkanAPI::Queue& graph_queue);
		static VulkanAPI::Texture generate_irradiance_map(vk::Device device, vk::PhysicalDevice& gpu, VulkanAPI::Queue& graph_queue);
	}
}