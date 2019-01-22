#pragma once

#include "Vulkan/Device.h"
#include "Vulkan/Interface.h"
#include "Vulkan/Shader.h"
#include "Vulkan/Pipeline.h"
#include "RenderConfig.h"
#include "RenderableTypes/RenderableBase.h"

#include <vector>
#include <memory>
#include <functional>

// forward decleartions
namespace VulkanAPI
{
	class DescriptorLayout;
	class CommandBuffer;
};

namespace OmegaEngine
{	
	// forward decleartions
	class ComponentInterface;
	class Object;
	class DeferredRenderer;
	enum class RenderTypes;
	class RenderableBase;
	class PostProcessInterface;

	struct RenderPipeline
	{
		VulkanAPI::Shader shader;
		VulkanAPI::PipelineLayout pl_layout;
		VulkanAPI::Pipeline pipeline;
		std::unique_ptr<VulkanAPI::DescriptorLayout> descr_layout;
	};

	// contain each stage of the render pipeline in the order in which to execute - each stage has its own framebuffer
	enum class RenderStage
	{
		GBuffer,		// offscreen g-buffer fill
		Deferred,		// renders to present queue if post-process not required
		PostProcess,	// rendered in a forward_pass
		Count
	};

	// Note: only deferred renderer is supported at the moment. More to follow....
	enum class RendererType
	{
		Deferred,
		Tile_Based,
		Count
	};


	class RenderInterface
	{

	public:

		// expand the renderables to include other associated components
		struct RenderableInfo
		{
			RenderableBase* renderable;
		};

		RenderInterface(VulkanAPI::Device device, const uint32_t win_width, const uint32_t win_height);
		~RenderInterface();

		// renderable type creation
		template <typename T, typename... Args>
		void add_renderable(RenderStage stage, Args&&... args)
		{
			std::unique_ptr<T> renderable = make_unique<T>(std::forward<Args>(args));
			renderables[stage].push_back(std::move(renderable));
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

		void render(double interpolation);
		void render_components();


	private:

		RenderConfig render_config;

		// pointers to each possible renderer. TODO: find a better way so we only have one pointer
		std::unique_ptr<DeferredRenderer> def_renderer;

		std::unique_ptr<VulkanAPI::Interface> vk_interface;
		std::unique_ptr<PostProcessInterface> postprocess_interface;

		// contains all objects that are renderable to the screen
		std::vector<RenderableInfo> renderables;

		// all the pipelines and shaders for each renderable type
		std::array<RenderPipeline, (int)OmegaEngine::RenderTypes::Count> render_pipelines;

		// the rendering call to use determined by which renderer is specified
		std::function<void()> render_callback;

		// Vulkan stuff for rendering the compoennts
		VulkanAPI::CommandBuffer& cmd_buffer;
	};

}

