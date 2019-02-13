#pragma once

#include "Vulkan/Device.h"
#include "Vulkan/Interface.h"
#include "Vulkan/Descriptors.h"
#include "Vulkan/Shader.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/CommandBuffer.h"
#include "RenderConfig.h"
#include "RenderableTypes/RenderableBase.h"

#include <vector>
#include <memory>
#include <functional>


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
		VulkanAPI::DescriptorLayout descr_layout;
		VulkanAPI::DescriptorSet descr_set;

		// information extracted from shader reflection
		std::vector<VulkanAPI::ShaderBufferLayout> buffer_layout;
		std::vector<VulkanAPI::ShaderImageLayout> image_layout;
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
			RenderableInfo(RenderableBase* rend) :
				renderable(rend)
			{}

			RenderableBase* renderable;
		};

		RenderInterface();
		RenderInterface(VulkanAPI::Device device, std::unique_ptr<ComponentInterface>& component_interface);
		~RenderInterface();

		// if expecting an object to have child objects (in the case of meshes for example), then use this function
		// this avoids having to iterate over a node tree, as we are linearising the tree so we can render faster and in sorted order
		void add_mesh_tree(std::unique_ptr<ComponentInterface>& comp_interface, Object& obj);

		// renderable type creation
		template <typename T, typename... Args>
		void add_renderable(Args&&... args)
		{
			T* renderable = new T(std::forward<Args>(args)...);
			renderables.push_back({ renderable });
		}

		void load_render_config();

		void init_renderer(std::unique_ptr<ComponentInterface>& interface);
		void init_environment_render();

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
		VulkanAPI::CommandBuffer cmd_buffer;
	};

}

