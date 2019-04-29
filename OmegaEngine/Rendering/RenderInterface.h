#pragma once
#include "Rendering/Renderers/RendererBase.h"
#include "Vulkan/Device.h"
#include "Vulkan/Datatypes/Texture.h"
#include "Vulkan/Interface.h"
#include "Vulkan/Descriptors.h"
#include "Vulkan/Shader.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/CommandBufferManager.h"
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

	template <typename FuncReturn, typename T, FuncReturn(T::*callback)(VulkanAPI::SecondaryCommandBuffer& cmd_buffer, void* renderable_data, RenderInterface* render_interface)>
	FuncReturn get_member_render_function(void *object, VulkanAPI::SecondaryCommandBuffer& cmd_buffer, void* renderable_data, RenderInterface* render_interface)
	{
		return (reinterpret_cast<T*>(object)->*callback)(cmd_buffer, renderable_data, render_interface);
	}

	enum class SceneType
	{
		Static,
		Dynamic
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
		RenderInterface(VulkanAPI::Device& device, std::unique_ptr<ComponentInterface>& component_interface, const uint32_t width, const uint32_t height, SceneType type);
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

		std::unique_ptr<ProgramState>& get_render_pipeline(RenderTypes type)
		{
			return render_states[(int)type];
		}

		void load_render_config();

		void init_renderer(std::unique_ptr<ComponentInterface>& component_interface);
		void init_environment_render();

		// shader init for each renderable type
		void add_shader(RenderTypes type, std::unique_ptr<ComponentInterface>& component_interface);

		void render(double interpolation);
		void render_components(RenderConfig& render_config, VulkanAPI::RenderPass& renderpass);

	private:

		RenderConfig render_config;
	
		// states whether the scene is static, i.e. no additional will be drawn, or dynamic
		SceneType scene_type;

		// handle to the cmd buffer 
		VulkanAPI::CmdBufferHandle cmd_buffer_handle;

		// pointers to each possible renderer. TODO: find a better way so we only have one pointer
		std::unique_ptr<RendererBase> renderer;

		std::unique_ptr<VulkanAPI::Interface> vk_interface;
		std::unique_ptr<PostProcessInterface> postprocess_interface;
		std::unique_ptr<RenderQueue> render_queue;

		// contains all objects that are renderable to the screen
		std::vector<RenderableInfo> renderables;

		// dirty flag indicates whether to rebuild the renderables
		bool isDirty = true;

		// all the pipelines and shaders for each renderable type
		std::array<std::unique_ptr<ProgramState>, (int)OmegaEngine::RenderTypes::Count> render_states;

	};

}

