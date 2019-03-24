#pragma once
#include "Rendering/Renderers/RendererBase.h"
#include "Vulkan/Device.h"
#include "Vulkan/Datatypes/Texture.h"
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
	class RenderQueue;
	class ObjectManager;

	template <typename FuncReturn, typename T, FuncReturn(T::*callback)(VulkanAPI::CommandBuffer& cmd_buffer, void* renderable_data, RenderInterface* render_interface, uint32_t thread)>
	FuncReturn get_member_render_function(void *object, VulkanAPI::CommandBuffer& cmd_buffer, void* renderable_data, RenderInterface* render_interface, uint32_t thread)
	{
		return (reinterpret_cast<T*>(object)->*callback)(cmd_buffer, renderable_data, render_interface, thread);
	}

	// contain each stage of the render pipeline in the order in which to execute - each stage has its own framebuffer
	enum class RenderStage
	{
		GBuffer,		// offscreen g-buffer fill
		Deferred,		// renders to present queue if post-process not required
		PostProcess,	// rendered in a forward_pass
		Count
	};

	class RenderInterface
	{

	public:

		struct ProgramState
		{
			VulkanAPI::Shader shader;
			VulkanAPI::PipelineLayout pl_layout;
			VulkanAPI::Pipeline pipeline;
			VulkanAPI::DescriptorLayout descr_layout;
			VulkanAPI::DescriptorSet descr_set;

			// information extracted from shader reflection
			std::vector<VulkanAPI::ShaderBufferLayout> buffer_layout;
			VulkanAPI::ImageLayoutBuffer image_layout;
		};

		// expand the renderables to include other associated components
		struct RenderableInfo
		{
			RenderableInfo(RenderableBase* rend) :
				renderable(rend)
			{}

			RenderableBase* renderable;
		};

		RenderInterface();
		RenderInterface(VulkanAPI::Device& device, std::unique_ptr<ComponentInterface>& component_interface, const uint32_t width, const uint32_t height);
		~RenderInterface();

		void init(VulkanAPI::Device& device, const uint32_t width, const uint32_t height);

		// if expecting an object to have child objects (in the case of meshes for example), then use this function
		// this avoids having to iterate over a node tree, as we are linearising the tree so we can render faster and in sorted order
		void build_renderable_tree(Object& obj, std::unique_ptr<ComponentInterface>& comp_interface);

		// adds a list of objects to the tree using the function above
		void update_renderables(std::unique_ptr<ObjectManager>& objecct_manager, std::unique_ptr<ComponentInterface>& comp_interface);

		// renderable type creation
		template <typename T, typename... Args>
		void add_renderable(Args&&... args)
		{
			T* renderable = new T(std::forward<Args>(args)...);
			renderables.push_back({ renderable });
		}

		template<typename T, typename... Args>
		void set_renderer(Args&&... args)
		{
			renderer = std::make_unique<T>(std::forward<Args>(args)...);
		}

		ProgramState& get_render_pipeline(RenderTypes type)
		{
			return render_pipelines[(int)type];
		}

		void load_render_config();

		void init_renderer(std::unique_ptr<ComponentInterface>& component_interface);
		void init_environment_render();

		// shader init for each renderable type
		void add_shader(RenderTypes type, std::unique_ptr<ComponentInterface>& component_interface);

		void render(double interpolation);
		void render_components(RenderConfig& render_config, VulkanAPI::RenderPass& renderpass, vk::Semaphore& image_semaphore, vk::Semaphore& component_semaphore);

	private:

		RenderConfig render_config;

		// pointers to each possible renderer. TODO: find a better way so we only have one pointer
		std::unique_ptr<RendererBase> renderer;

		std::unique_ptr<VulkanAPI::Interface> vk_interface;
		std::unique_ptr<PostProcessInterface> postprocess_interface;
		std::unique_ptr<RenderQueue> render_queue;

		// contains all objects that are renderable to the screen
		std::vector<RenderableInfo> renderables;

		// dirty flag indicates whether to rebuild the renderables
		bool isDirty = true;

		// indicates whether the cmd buffers need rebuilding
		bool rebuildCmdBuffers = false;

		// all the pipelines and shaders for each renderable type
		std::array<ProgramState, (int)OmegaEngine::RenderTypes::Count> render_pipelines;

		// Vulkan stuff for rendering the compoennts
		VulkanAPI::CommandBuffer cmd_buffer;

	public:

		// preperation for final swapchain presentation
		void prepare_swapchain_pass();
		VulkanAPI::CommandBuffer& begin_swapchain_pass(uint32_t index);
		void end_swapchain_pass(uint32_t index);

		VulkanAPI::RenderPass& get_swapchain_renderpass()
		{
			return swapchain_present.renderpass;
		}

		uint32_t get_swapchain_count() const
		{
			return swapchain_present.cmd_buffer.size();
		}

		VulkanAPI::CommandBuffer& get_sc_cmd_buffer(uint32_t index)
		{
			return swapchain_present.cmd_buffer[index];
		}

	private:

		// final presentation pass
		struct SwapChainPresentaion
		{
			std::vector<VulkanAPI::CommandBuffer> cmd_buffer;
			VulkanAPI::RenderPass renderpass;
			VulkanAPI::Texture depth_texture;

			std::array<vk::ClearValue, 2> clear_values = {};
		} swapchain_present;

	};

}

