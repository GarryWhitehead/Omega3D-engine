#pragma once

#include "Core/ObjectManager.h"

#include "Models/Formats/GltfModel.h"

#include <vector>

namespace OmegaEngine
{

// forward decleartions
class World;
class GltfModel;
class Object;
class MaterialInfo;

class Scene
{
public:
	Scene() = default;

	Scene(World& world);
	~Scene();

	// as scene isn't copyable
	Scene(Scene&) = delete;
	Scene& operator=(Scene&) = delete;

	// but is moveable
	Scene(Scene&&) = default;
	Scene& operator=(Scene&&) = default;

	void buildModel(GltfModel& model, World& world);

	void buildRenderableMeshTree(Object& obj);
	void update();

	/**
	* Gets a renderable type from the list
	* @param index the index of the renderable type
	* @return A renderable type - this will be as the base class
	*/
	RenderableInfo& getRenderable(uint32_t index)
	{
		assert(index < renderables.size());
		return renderables[index];
	}

	/**
	* Return an instance of the object manager
	* Used by the user for creating objects - usually passed to the scene
	*/
	ObjectManager& getObjManager()
	{
		return objManager;
	}

private:
	// private functions to the scene

	/**
	* Adds a renderable type to the list.
	* @param T renderable type
	* @param args argument list which matches the init list for this particular renderable
	*/
	template <typename T, typename... Args>
	void addRenderable(Args&&... args)
	{
		T* renderable = new T(std::forward<Args>(args)...);
		renderables.push_back({ renderable });
	}

private:
	/// objects assoicated with this scene dealt with by the object manager
	ObjectManager objManager;

	/// contains all the renderable objects associated with this scene.
	/// These have gone through visibilty checks and basically wrap the 
	/// data stored by the renderable manager which will be exposed to
	/// the renderer.
	std::vector<RenderableBase*> renderables;

	/// The world this scene is assocaited with
	/// Should be safe, as the scene will be deleted before the world
	World& world;
};
}    // namespace OmegaEngine