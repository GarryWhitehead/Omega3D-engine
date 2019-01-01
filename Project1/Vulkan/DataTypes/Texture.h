#pragma once
#include "Vulkan/Common.h"

namespace VulkanAPI
{
	// forward declerations
	class MemoryAllocator;

	class Texture
	{

	public:

		Texture();
		~Texture();

		void map(OmegaEngine::MappedTexture& tex, std::unique_ptr<MemoryAllocator>& mem_alloc);

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;
	};

}

