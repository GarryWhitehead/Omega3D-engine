#pragma once
#include "Rendering/Renderers/RendererBase.h"
#include "RenderConfig.h"
#include "RenderableTypes/RenderableBase.h"

#include <vector>
#include <memory>
#include <functional>

// forward declerations
namespace VulkanAPI
{
	class Device;
	class Interface;
}

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

	template <typename FuncReturn, typename T, FuncReturn(T::*callback)(VulkanAPI::SecondaryCommandBuffer& cmd_buffer, void* renderable_data)>
	FuncReturn get_member_render_function(void *object, VulkanAPI::SecondaryCommandBuffer& cmd_buffer, void* renderable_data)
	{
		return (reinterpret_cast<T*>(object)->*callback)(cmd_buffer, renderable_data);
	}

	enum class SceneType
	{
		Static,
		Dynamic
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
		RenderInterface(std::unique_ptr<VulkanAPI::Device>& device, const uint32_t width, const uint32_t height, SceneType type);
		~RenderInterface();

		void init(std::unique_ptr<VulkanAPI::Device>& device, const uint32_t width, const uint32_t height);

		// if expecting an object to have child objects (in the case of meshes for example), then use this function
		// this avoids having to iterate over a node tree, as we are linearising the tree so we can render faster and in sorted order
		void build_renderable_mesh_tree(Object& obj, std::unique_ptr<ComponentInterface>& comp_interface, bool is_shadow);

		// adds a list of objects to the tree using the function above
		void update_renderables(std::unique_ptr<ObjectManager>& objecct_manager, std::unique_ptr<ComponentInterface>& comp_interface);

		// renderable type creation
		template <typename T, typename... Args>
		uint32_t add_renderable(Args&&... args)
		{
			T* renderable = new T(std::forward<Args>(args)...);
			renderables.push_back({ renderable });
			return static_cast<uint32_t>(renderables.size() - 1);
		}

		RenderableInfo& get_renderable(uint32_t index)
		{
			assert(index < renderables.size());
			return renderables[index];
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

		void init_renderer(std::unique_ptr<ComponentInterface>& component_interface);
		void init_environment_render();

		// shader init for each renderable type
		void add_shader(RenderTypes type, std::unique_ptr<ComponentInterface>& component_interface);

		// adds all renderables to render queue - TODO: add visisbility check
		void prepare_object_queue();

		// renders the frame using the defined renderer
		void render(double interpolation);

	private:

		RenderConfig render_config;
	
		// states whether the scene is static, i.e. no additional will be drawn, or dynamic
		SceneType scene_type;

		// pointers to each possible renderer. TODO: find a better way so we only have one pointer
		std::unique_ptr<RendererBase> renderer;

		std::unique_ptr<VulkanAPI::Interface> vk_interface;
		std::unique_ptr<PostProcessInterface> postprocess_interface;

		// contains all objects that are renderable to the screen
		std::vector<RenderableInfo> renderables;

		// queued visible renderables
		std::unique_ptr<RenderQueue> render_queue;

		// dirty flag indicates whether to rebuild the renderables
		bool isDirty = true;

		// all the pipelines and shaders for each renderable type
		std::array<std::unique_ptr<ProgramState>, (int)OmegaEngine::RenderTypes::Count> render_states;

	};

}

