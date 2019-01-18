#pragma once

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
	class RenderableBase;

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
			std::unique_ptr<T> renderable = make_unique<T>(std::forward<Args>(args));
			renderables.push_back(std::move(renderable));
		}

		// if expecting an object to have child objects (in the case of meshes for example), then use this function
		// this avoids having to iterate over a node tree, as we are linearising the tree so we can render faster and in sorted order
		template <typename T>
		void addObjectAndChildren(std::unique_ptr<ComponentInterface>& comp_interface, Object& obj)
		{
			add_renderable<T>(comp_interface, obj);
			
			auto& children = obj.get_children();
			for (auto& child : children) {
				addObjectAndChildren<T>(comp_interface, child);
			}
		}

		// shader init for each renderable type
		void add_shader(RenderTypes type);

		void render();


	private:

		std::unique_ptr<Renderer> renderer;
		std::unique_ptr<VulkanAPI::Interface> vk_interface;

		// contains all objects that are renderable to the screen
		std::vector<std::unique_ptr<RenderableBase> > renderables;

		// all the pipelines and shaders for each renderable type
		std::array<RenderPipeline, (int)OmegaEngine::RenderTypes::Count> render_pipelines;
	};

}

