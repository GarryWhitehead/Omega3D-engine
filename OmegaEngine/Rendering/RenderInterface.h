#pragma once
#include "Rendering/Renderers/RendererBase.h"
#include "Rendering/RenderConfig.h"
#include "RenderableTypes/RenderableBase.h"
#include "Rendering/StockModels.h"

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

	template <typename FuncReturn, typename T, FuncReturn(T::*callback)(VulkanAPI::SecondaryCommandBuffer& cmdBuffer, void* renderableData)>
	FuncReturn getMemberRenderFunction(void *object, VulkanAPI::SecondaryCommandBuffer& cmdBuffer, void* renderableData)
	{
		return (reinterpret_cast<T*>(object)->*callback)(cmdBuffer, renderableData);
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
		void buildRenderableMeshTree(Object& obj, std::unique_ptr<ComponentInterface>& componentInterface, bool is_shadow);

		// adds a list of objects to the tree using the function above
		void updateRenderables(std::unique_ptr<ObjectManager>& objecctManager, std::unique_ptr<ComponentInterface>& componentInterface);

		// renderable type creation
		template <typename T, typename... Args>
		uint32_t addRenderable(Args&&... args)
		{
			T* renderable = new T(std::forward<Args>(args)...);
			renderables.push_back({ renderable });
			return static_cast<uint32_t>(renderables.size() - 1);
		}

		RenderableInfo& getRenderable(uint32_t index)
		{
			assert(index < renderables.size());
			return renderables[index];
		}

		template<typename T, typename... Args>
		void setRenderer(Args&&... args)
		{
			renderer = std::make_unique<T>(std::forward<Args>(args)...);
		}

		std::unique_ptr<ProgramState>& getRenderPipeline(RenderTypes type)
		{
			return renderStates[(int)type];
		}

		void initRenderer(std::unique_ptr<ComponentInterface>& componentInterface);

		// shader init for each renderable type
		void addShader(RenderTypes type, std::unique_ptr<ComponentInterface>& componentInterface);

		// adds all renderables to render queue - TODO: add visisbility check
		void prepareObjectQueue();

		// renders the frame using the defined renderer
		void render(double interpolation);

	private:

		RenderConfig renderConfig;
	
		// states whether the scene is static, i.e. no additional will be drawn, or dynamic
		SceneType sceneType;

		// pointers to each possible renderer. TODO: find a better way so we only have one pointer
		std::unique_ptr<RendererBase> renderer;

		std::unique_ptr<VulkanAPI::Interface> vkInterface;
		std::unique_ptr<PostProcessInterface> postProcessInterface;

		// contains all objects that are renderable to the screen
		std::vector<RenderableInfo> renderables;

		// queued visible renderables
		std::unique_ptr<RenderQueue> renderQueue;

		// dirty flag indicates whether to rebuild the renderables
		bool isDirty = true;

		// all the pipelines and shaders for each renderable type
		std::array<std::unique_ptr<ProgramState>, (int)OmegaEngine::RenderTypes::Count> renderStates;

		// stock models
		std::unique_ptr<RenderUtil::CubeModel> cubeModel;
		std::unique_ptr<RenderUtil::PlaneModel> planeModel;

	};

}

