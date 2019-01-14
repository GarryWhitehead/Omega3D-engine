#pragma once

#include "Rendering/RenderableTypes.h"
#include "Vulkan/Device.h"
#include "Vulkan/Interface.h"
#include "Vulkan/Shader.h"
#include "Vulkan/Pipeline.h"

#include <vector>
#include <memory>

// forward decleartions
namespace VulkanAPI
{
	class DescriptorLayout;
};

namespace OmegaEngine
{	
	// forward decleartions
	class ComponentInterface;
	class Object;
	class Renderer;
	enum class RenderTypes;

	struct RenderPipeline
	{
		VulkanAPI::Shader shader;
		VulkanAPI::PipelineLayout pl_layout;
		VulkanAPI::Pipeline pipeline;
		std::unique_ptr<VulkanAPI::DescriptorLayout> descr_layout;
	};

	class RenderInterface
	{

	public:

		RenderInterface(VulkanAPI::Device device, const uint32_t win_width, const uint32_t win_height);
		~RenderInterface();

		// renderable type creation
		template <typename T, typename... Args>
		void add_renderable(Args&&... args)
		{
			T renderable(std::forward<Args>(args));
			renderables.push_back(renderable);
		}

		// shader init for each renderable type
		void add_shader(RenderTypes type);

	private:

		std::unique_ptr<Renderer> renderer;
		std::unique_ptr<VulkanAPI::Interface> vk_interface;

		// contains all objects that are renderable to the screen
		std::vector<RenderableType> renderables;

		// all the pipelines and shaders for each renderable type
		std::array<RenderPipeline, (int)OmegaEngine::RenderTypes::Count> pipelines;
	};

}

