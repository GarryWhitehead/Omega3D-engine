#pragma once
#include "Vulkan/Common.h"
#include "Vulkan/CommandBuffer.h"

// forward declerations

namespace OmegaEngine
{

	struct PostProcessInfo
	{
		// render callback function
        void (*renderFunction)(void* postProcessData);
		void *postProcessHandle;

        // data specific to the renderable - mainly drawing information 
        void *postProcessData;
	}

	class PostProcessInterface
	{

	public:

		PostProcessInterface(vk::Device dev);
		~PostProcessInterface();

		void init(RenderConfig& renderConfig);
		void render();

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;
		
		std::vector<PostProcessInfo> postProcessPasses;
	};

}

