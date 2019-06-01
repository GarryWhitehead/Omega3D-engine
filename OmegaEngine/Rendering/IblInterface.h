#pragma once

#include "Vulkan/Common.h"
#include "Vulkan/DataTypes/Texture.h"
#include "OEMaths/OEMaths.h"

#include <vector>

namespace VulkanAPI
{
	class Queue;
}

namespace OmegaEngine
{
	class RenderConfig;

	class IblInterface
	{

	public:

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

		IblInterface(vk::Device device, vk::PhysicalDevice& gpu, VulkanAPI::Queue& graphicsQueue);
		~IblInterface();

		void generateBrdf(vk::Device device, vk::PhysicalDevice& gpu, VulkanAPI::Queue& graphicsQueue);
		void generateIrradianceMap(vk::Device device, vk::PhysicalDevice& gpu, VulkanAPI::Queue& graphicsQueue);

	private:

		VulkanAPI::Texture brdfTexture;
		VulkanAPI::Texture irradianceMapTexture;

	};

}


